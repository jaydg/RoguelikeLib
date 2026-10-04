/* A simple name generator for a roguelike game.
 * Written by Kusigrosz, February-May 2010. This program is in public domain.
 *
 * ./a.out < input_file
 * ./a.out mintokens maxtokens < input_file
 *
 * The algorithm:
 * Words read from the standard input are scanned for tokens of the
 * form vowel_prefix-consonant_suffix or consonant_prefix-vowel_suffix.
 * The beginning and the end of the word are treated as additional
 * vowels '<' and '>'. However, a lone '>' is not regarded as an ending
 * vowel suffix: it is appended to the previous token - see the special
 * case in tokenize().
 * For example, "Elizabeth" is tokenized:
 * <el - li - iz - za - ab - be - eth>.
 *
 * After all the input is read and the tokens stored, calling
 * nam_generate(sbuf, mintokens, maxtokens, buflen) attempts to
 * generate a name made of between mintokens and maxtokens tokens in
 * the provided buffer sbuf of length buflen. The first token is
 * selected randomly from tokens that start with '<'. Each next token
 * is selected randomly from tokens whose prefix matches the
 * suffix of the previous one. The next token, minus the matching
 * prefix, is appended to the already constructed part of the
 * word. Tokens that end words (their last char is '>') are allowed
 * only if the token count reaches mintokens - 1; they are the only
 * ones allowed if maxtokens - 1 is reached.
 *
 * The random selection takes into account the number of occurrences
 * of the given token in the input words; the probability that the
 * given matching token is selected is proportional to this number.
 *
 * Note that a call to nam_generate() may fail; there is a finite
 * number of attempts made, and it may happen that all of them make
 * word beginnings that can't be ended within the given constraints.
 * Testing with the input of about a hundred popular english names,
 * mintoken = 4 and maxtoken = 6 suggests such failure is very rare.
 * Even number of tokens generates words ending in vowels, odd ones
 * in consonants; to allow both, the range from mintokens to maxtokens
 * must contain both even and odd numbers.
 */

//////////////////////////////////////////////////////////////////////////
// Names
//
// Kusigrosz's name generator (y_names23b.c), as a class: words are given
// to CNameGenerator with AddWord() or AddWords() rather than read from the
// standard input, and Generate() makes a name of them, or says it could
// not. nam_generate() maps to Generate(), tokenize() to Tokenize(). The
// numbers come from RL::Random(), so a seeded run makes the same names.
//
// Names of women and of men sound different, so CPersonNames keeps a
// generator for each, learned from names of that gender: common English
// given names, unless it is given others.
//
// Unlike the original, a word may be of any length and so may its tokens:
// there are no buffers to fit. Only the letters A to Z make words; any
// other character, accented letters included, ends one.
//////////////////////////////////////////////////////////////////////////

module;

export module rl.names;

import rl.randomness;
import std;

export namespace RL
{

class CNameGenerator
{
private:
    // The vowels, the start and end of a word included as '<' and '>'
    static constexpr std::string_view vowels = "<>aeiouy";
    static constexpr std::string_view consonants = "bcdfghjklmnpqrstvwxz";

    // How many attempts Generate() makes before it gives up
    static constexpr int max_tries = 100;

    struct SToken {
        // Where the suffix starts
        std::size_t suffix = 0;

        // How many times it was found in the words
        unsigned count = 0;
    };

    // Each token by its text: "<el", "li", "eth>"
    std::map<std::string, SToken, std::less<>> tokens;
    std::size_t words = 0;

    // Whether a token may, may not or has to end a word
    enum class EEnd { Cannot, May, Must };

    // A random token whose prefix is the given text, or one that starts a
    // word if the text is empty, chosen in proportion to how often each
    // was found; and which can, may or must end a word as asked. Nothing
    // if there is none.
    [[nodiscard]] const std::pair<const std::string, SToken>* Match(std::string_view prefix, EEnd end) const
    {
        const std::string_view start = prefix.empty() ? std::string_view("<") : prefix;
        const std::pair<const std::string, SToken>* chosen = nullptr;
        unsigned total = 0;

        for (auto it = tokens.lower_bound(start); it != tokens.end() && it->first.starts_with(start); ++it) {
            const auto& [text, token] = *it;

            if (!prefix.empty() && token.suffix != prefix.size()) {
                continue;
            }

            const bool ending = text.back() == '>';

            if ((end == EEnd::Cannot && ending) || (end == EEnd::Must && !ending)) {
                continue;
            }

            // Each in proportion to its count, in a single pass
            total += token.count;

            if (Random(total) < token.count) {
                chosen = &*it;
            }
        }

        return chosen;
    }

    static bool IsLetter(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    // The tokens of a word, each with where its suffix starts
    static std::vector<std::pair<std::string, std::size_t>> Split(std::string_view word)
    {
        std::vector<std::pair<std::string, std::size_t>> result;

        if (word.empty() || !std::ranges::all_of(word, IsLetter)) {
            return result;
        }

        std::string marked = "<";

        for (const char c : word) {
            marked += static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
        }

        marked += '>';

        // Words start with the '<' "vowel"
        std::size_t start = 0;
        std::size_t prefix = std::min(marked.find_first_not_of(vowels), marked.size());

        while (prefix > 0) {
            // A vowel prefix takes the consonants after it, a consonant
            // prefix the vowels
            const bool vowel_token = vowels.find(marked[start]) != std::string_view::npos;
            const std::size_t suffix_start = start + prefix;
            const std::size_t suffix_end = std::min(
                                               marked.find_first_not_of(vowel_token ? consonants : vowels, suffix_start), marked.size());
            std::size_t suffix = suffix_end - suffix_start;

            if (suffix == 0) {
                break;
            }

            // A lone '>' after the token is taken into it
            if (suffix_end < marked.size() && marked[suffix_end] == '>') {
                ++suffix;
            }

            result.emplace_back(marked.substr(start, prefix + suffix), prefix);
            start += prefix;
            prefix = suffix;
        }

        return result;
    }

public:
    // Splits a word into its tokens, its start marked with '<' and its end
    // with '>': "Elizabeth" is "<el", "li", "iz", "za", "ab", "be", "eth>".
    // A word of anything but the letters A to Z has none.
    [[nodiscard]] static std::vector<std::string> Tokenize(std::string_view word)
    {
        std::vector<std::string> texts;

        for (auto& [text, suffix] : Split(word)) {
            texts.push_back(std::move(text));
        }

        return texts;
    }

    // Learns the tokens of a word, which counts only if it is made of the
    // letters A to Z. Returns whether it did.
    bool AddWord(std::string_view word)
    {
        const auto found = Split(word);

        if (found.empty()) {
            return false;
        }

        for (const auto& [text, suffix] : found) {
            auto it = tokens.find(text);

            if (it == tokens.end()) {
                it = tokens.emplace(text, SToken{suffix, 0}).first;
            }

            ++it->second.count;
        }

        ++words;
        return true;
    }

    // Learns every word of a text: runs of the letters A to Z, between
    // anything else. Returns how many there were.
    std::size_t AddWords(std::string_view text)
    {
        std::size_t added = 0;
        std::size_t pos = 0;

        while (pos < text.size()) {
            while (pos < text.size() && !IsLetter(text[pos])) {
                ++pos;
            }

            std::size_t end = pos;

            while (end < text.size() && IsLetter(text[end])) {
                ++end;
            }

            if (end > pos && AddWord(text.substr(pos, end - pos))) {
                ++added;
            }

            pos = end;
        }

        return added;
    }

    // Makes a name of between `min_tokens` and `max_tokens` tokens, and no
    // longer than `max_length` letters, capitalised. The original was
    // limited by its buffer of 256 characters; 30 letters are more than
    // enough for a name. Nothing if no attempt
    // came out within those limits, which can happen; see above. Throws if
    // there cannot be such a name.
    [[nodiscard]] std::optional<std::string> Generate(std::size_t min_tokens, std::size_t max_tokens,
                                                      std::size_t max_length = 30) const
    {
        if (min_tokens == 0 || max_tokens < min_tokens) {
            throw std::invalid_argument("a name needs at least one token, and no fewer than it has at least");
        }

        for (int tries = 0; tries < max_tries; ++tries) {
            std::string name;

            // Where the suffix of the last token starts
            std::size_t suffix = 0;
            std::size_t count = 0;
            bool ended = false;

            while (!ended) {
                const EEnd end = count + 1 >= max_tokens ? EEnd::Must : count + 1 >= min_tokens ? EEnd::May : EEnd::Cannot;
                const auto* token = Match(std::string_view(name).substr(suffix), end);

                // The name so far ends with the suffix the token starts
                // with; with the '<' and '>', it is two longer than it
                // will be
                if (token == nullptr || suffix + token->first.size() > max_length + 2) {
                    break;
                }

                const auto& [text, data] = *token;
                name += count == 0 ? text : text.substr(data.suffix);
                suffix += data.suffix;
                ended = text.back() == '>';
                ++count;
            }

            if (ended) {
                // Without the start and end, capitalised
                name = name.substr(1, name.size() - 2);
                name[0] = static_cast<char>(name[0] - 'a' + 'A');
                return name;
            }
        }

        return std::nullopt;
    }

    // How many words have been learned
    [[nodiscard]] std::size_t getWordCount() const
    {
        return words;
    }

    // How many different tokens they had
    [[nodiscard]] std::size_t getTokenCount() const
    {
        return tokens.size();
    }
};

enum class EGender {
    Female,
    Male
};

// Common English given names of women and of men, to learn from
constexpr std::string_view english_female_names =
    "Abigail Agnes Alice Amelia Anna Ava Beatrice Catherine Charlotte Clara Dorothy Edith Eleanor Elizabeth "
    "Ella Emily Emma Evelyn Florence Grace Hannah Harriet Helen Irene Isabella Jane Jessica Joan Julia "
    "Katherine Laura Lily Lucy Margaret Maria Martha Mary Matilda Mildred Nancy Olivia Patricia Rachel "
    "Rebecca Rose Ruth Sarah Sophia Susan Victoria";

constexpr std::string_view english_male_names =
    "Adam Albert Alexander Alfred Andrew Anthony Arthur Benjamin Bernard Charles Christopher Daniel David "
    "Edmund Edward Francis Frederick Geoffrey George Gilbert Harold Harry Henry Hugh Jack Jacob James John "
    "Joseph Joshua Leonard Martin Matthew Michael Nicholas Oliver Oscar Paul Peter Philip Ralph Richard "
    "Robert Roger Samuel Simon Stephen Thomas Walter William";

// Names for people of either gender, each made up from names of people of
// that gender
class CPersonNames
{
private:
    CNameGenerator female;
    CNameGenerator male;

public:
    // Learns the names of women and of men from texts of them, the common
    // English ones by default
    explicit CPersonNames(std::string_view female_names = english_female_names,
                          std::string_view male_names = english_male_names)
    {
        female.AddWords(female_names);
        male.AddWords(male_names);
    }

    // Makes a name for a person of a gender; see CNameGenerator::Generate()
    [[nodiscard]] std::optional<std::string> Generate(EGender gender, std::size_t min_tokens = 4,
                                                      std::size_t max_tokens = 6, std::size_t max_length = 30) const
    {
        return getGenerator(gender).Generate(min_tokens, max_tokens, max_length);
    }

    // The generator of names of a gender, e.g. to teach it more
    [[nodiscard]] const CNameGenerator& getGenerator(EGender gender) const
    {
        return gender == EGender::Female ? female : male;
    }

    [[nodiscard]] CNameGenerator& getGenerator(EGender gender)
    {
        return gender == EGender::Female ? female : male;
    }
};

} // namespace RL
