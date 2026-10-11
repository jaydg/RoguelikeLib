// Tests for the name generator: words split into tokens, and names made of
// them again

import rl.names;
import rl.randomness;
import rltests.check;
import std;

using RL::tests::Check;

namespace
{

using Tokens = std::vector<std::string>;

// Fifty names for people of a gender, as many as can be made
std::vector<std::string> Generate(const RL::CPersonNames& people, RL::EGender gender)
{
    std::vector<std::string> names;

    for (int count = 0; count < 50; ++count) {
        if (const auto name = people.Generate(gender)) {
            names.push_back(*name);
        }
    }

    return names;
}

// Every token of the words of a text
std::set<std::string> TokensOf(std::string_view text)
{
    std::set<std::string> tokens;

    for (const auto word : std::views::split(text, ' ')) {
        for (const std::string& token : RL::CNameGenerator::Tokenize(std::string_view(word.begin(), word.end()))) {
            tokens.insert(token);
        }
    }

    return tokens;
}

// Whether every token of every name is one of those given
bool MadeOf(const std::vector<std::string>& names, const std::set<std::string>& tokens)
{
    return std::ranges::all_of(names, [&tokens](const std::string & name) {
        return std::ranges::all_of(RL::CNameGenerator::Tokenize(name), [&tokens](const std::string & token) {
            return tokens.contains(token);
        });
    });
}

bool IsLowerCase(char c)
{
    return c >= 'a' && c <= 'z';
}

// A capital letter, then small ones, and no more than thirty
bool IsCapitalisedWord(const std::string& name)
{
    return !name.empty() && name.size() <= 30 && name[0] >= 'A' && name[0] <= 'Z' &&
           std::ranges::all_of(name.substr(1), IsLowerCase);
}

} // namespace

void TestNames()
{
    using RL::CNameGenerator;

    // Words into tokens, as the original describes it
    Check(CNameGenerator::Tokenize("Elizabeth") == Tokens{"<el", "li", "iz", "za", "ab", "be", "eth>"},
          "Elizabeth is <el - li - iz - za - ab - be - eth>");
    Check(CNameGenerator::Tokenize("Anna") == Tokens{"<ann", "nna>"}, "a word ending in a vowel has it in its last token");
    Check(CNameGenerator::Tokenize("ELI") == Tokens{"<el", "li>"}, "letters are taken in lower case");
    Check(CNameGenerator::Tokenize("").empty() && CNameGenerator::Tokenize("Jo3").empty() &&
          CNameGenerator::Tokenize("Zoë").empty(), "only the letters A to Z make words");
    Check(CNameGenerator::Tokenize("Aia").empty(), "a word of vowels alone has no tokens");

    CNameGenerator generator;
    Check(generator.AddWords("Elizabeth, 42 times!") == 2 && generator.getWordCount() == 2 &&
          generator.getTokenCount() == 7 + 5, "every run of letters in a text is a word");
    Check(!generator.AddWord("Eia") && generator.getWordCount() == 2, "a word without tokens is not learned");

    bool threw = false;

    try {
        static_cast<void>(generator.Generate(0, 3));
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    Check(threw, "a name has at least one token");

    // One word gives itself back: each token has only one to follow it
    CNameGenerator one;
    one.AddWord("Elizabeth");
    Check(one.Generate(7, 7) == "Elizabeth", "a single word, made again");
    Check(!one.Generate(1, 3).has_value(), "and no name when there is none of the length asked for");
    Check(!one.Generate(7, 7, 8).has_value() && one.Generate(7, 7, 9) == "Elizabeth", "nor one too long");

    // Tokens are chosen as often as they were found
    CNameGenerator weighted;
    weighted.AddWords("Ana Ana Ana Ama");
    int ana = 0;

    for (int count = 0; count < 400; ++count) {
        ana += weighted.Generate(2, 2) == "Ana" ? 1 : 0;
    }

    Check(ana > 250 && ana < 350, "a token found three times as often is chosen about three times as often");

    // Names for women and for men, each made from names of their gender:
    // capitalised, new ones among them, and the same again from the same
    // seed
    const RL::CPersonNames people;
    using RL::EGender;
    Check(people.getGenerator(EGender::Female).getWordCount() == 50 &&
          people.getGenerator(EGender::Male).getWordCount() == 50, "fifty names of women and fifty of men are learned");

    RL::InitRandomness(2010);
    const std::vector<std::string> women = Generate(people, EGender::Female);
    const std::vector<std::string> men = Generate(people, EGender::Male);
    RL::InitRandomness(2010);
    const std::vector<std::string> again = Generate(people, EGender::Female);

    Check(women.size() >= 48 && men.size() >= 48, "making a name rarely fails");
    Check(std::ranges::all_of(women, IsCapitalisedWord) && std::ranges::all_of(men, IsCapitalisedWord),
          "names are capitalised words");
    Check(MadeOf(women, TokensOf(RL::english_female_names)) && MadeOf(men, TokensOf(RL::english_male_names)),
          "a woman's name is made of names of women, a man's of names of men");
    Check(!MadeOf(women, TokensOf(RL::english_male_names)) && !MadeOf(men, TokensOf(RL::english_female_names)),
          "and not of the others'");
    Check(std::ranges::any_of(women, [](const std::string & name) {
        return RL::english_female_names.find(name) == std::string_view::npos;
    }), "some are new");
    Check(women == again, "the same seed makes the same names");

    // Names of others' choosing
    const RL::CPersonNames elves("Galadriel Arwen Luthien Nimrodel", "Elrond Legolas Thranduil Glorfindel");
    Check(elves.getGenerator(EGender::Female).getWordCount() == 4 && elves.Generate(EGender::Male, 1, 8).has_value(),
          "names can be learned from other names");
}
