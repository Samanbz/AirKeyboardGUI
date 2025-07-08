#pragma once

#include <windows.h>

/**
 * Represents the state of a character in the UI.
 * - BLANK: No input or state.
 * - CORRECT: The character was correctly input.
 * - WRONG: The character was incorrectly input.
 */
enum CharState {
    BLANK,
    CORRECT,
    WRONG
};

/**
 * Represents a single character in the UI that can react to user input.
 * Stores its position, state, and provides methods for drawing and updating state.
 */
class ReactiveChar {
private:
    /**
     * The character this object represents.
     */
    WCHAR character;

    /**
     * The current state of the character (blank, correct, or wrong).
     */
    CharState state;

    /**
     * Position of the character in the client coordinate space.
     * Not relative to any parent view, as characters do not have their own window or UIView.
     */
    POINTI position;

public:
    /**
     * Constructs a ReactiveChar with the given character and optional position.
     * @param ch The character to represent.
     * @param x The x-coordinate in client space (default 0).
     * @param y The y-coordinate in client space (default 0).
     */
    ReactiveChar(WCHAR ch, int x = 0, int y = 0)
        : character(ch), state(BLANK) {
        position = {x, y};
    }

    /**
     * Draws the character at its position using the provided device context.
     * The color reflects the current state: green for correct, red for wrong, black for blank.
     * @param hdc The device context to draw on.
     */
    void drawSelf(HDC hdc) {
        SetTextColor(hdc, (state == CORRECT) ? RGB(30, 30, 30) : (state == WRONG) ? RGB(219, 34, 31)
                                                                                  : RGB(120, 120, 130));
        TextOutW(hdc, position.x, position.y, &character, 1);
    }

    /**
     * Updates the state based on a keystroke.
     * Sets state to CORRECT if the input matches the character, otherwise WRONG.
     * @param ch The input character to compare.
     */
    void logKeyStroke(WCHAR ch) {
        state = (ch == character) ? CORRECT : WRONG;
    }

    void reset() {
        state = BLANK;
    }

    /**
     * Sets the position of the character relative to the text container.
     * @param x The new x-coordinate.
     * @param y The new y-coordinate.
     */
    void setPosition(int x, int y) {
        position.x = x;
        position.y = y;
    }

    /**
     * Sets the position of the character relative to the text container.
     * @return The current position of the character.
     */
    POINTI getPosition() const {
        return position;
    }

    /**
     * Returns the character represented by this object.
     * @return The WCHAR character.
     */
    WCHAR getCharacter() const {
        return character;
    }
};
