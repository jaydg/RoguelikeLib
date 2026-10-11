// Tests for RTUI's rtui.input: a line of text typed into a window

import rtui.input;
import rltests.check;
import std;

using RL::tests::Check;
using RL::tests::Holds;
using RL::tests::Row;

void TestInput()
{
    RL::CInputWindow input("Name", {RL::Position(0, 0), RL::Size(30, 3)}, 6);
    input.setPrompt("Name: ");
    const auto type = [&input](RL::EKey key, char32_t ch = 0) {
        return input.HandleKey(RL::SKey{key, ch, {}});
    };

    Check(type(RL::EKey::Char, U'Á') && type(RL::EKey::Char, U'd') && type(RL::EKey::Char, U'a') && input.getText() == "Áda",
          "characters typed are added, UTF-8 too");
    Check(type(RL::EKey::Backspace) && input.getText() == "Ád", "Backspace takes the last away");

    for (const char32_t c : std::u32string(U"<rich>")) {
        type(RL::EKey::Char, c);
    }

    Check(input.getText() == "Ád<ric", "no longer than allowed");
    Check(!type(RL::EKey::Up) && !type(RL::EKey::Char, U'\x03'), "what is no text is not taken");

    RL::CScreen screen(RL::Size(20, 1));
    input.Draw(screen, {RL::Position(0, 0), RL::Size(20, 1)});
    Check(Holds(Row(screen, 0), U"Name: Ád<ric_"), "the prompt, what is typed, as it is, and where the next goes");

    Check(!input.TakeEntered() && type(RL::EKey::Enter) && input.TakeEntered() == std::optional<std::string>("Ád<ric") &&
          !input.TakeEntered(), "Enter enters the text, once");
    Check(!input.TakeCancelled() && type(RL::EKey::Escape) && input.TakeCancelled() && !input.TakeCancelled(),
          "Escape cancels, once");

    input.setText("Walter Bright");
    Check(input.getText() == "Walter", "text set starts from there, no longer than allowed");
}
