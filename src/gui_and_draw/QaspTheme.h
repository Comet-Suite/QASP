//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
// QaspTheme.h
//
// QASP addition (not present in upstream OpenVSP).
//
// Applies a flat, modern visual theme.  Upstream OpenVSP relies on FLTK's
// default 3D-beveled box types for buttons, panels and inputs, which gives the
// interface the recessed, high-contrast look of mid-1990s desktop software.
// This replaces those bevels with flat fills and a single hairline border, and
// sets a cooler, lower-contrast palette.
//
// Deliberately conservative: it changes only colours and box drawing.  No
// widget is moved, resized, hidden or replaced, so layout cannot regress.
//

#ifndef QASP_THEME_H
#define QASP_THEME_H

class QaspTheme
{
public:

    // Applies the theme.  Call once, before any widgets are created.
    static void Apply();

    // True if the theme has been applied (useful for diagnostics).
    static bool IsApplied() { return s_Applied; }

private:

    static bool s_Applied;
};

#endif
