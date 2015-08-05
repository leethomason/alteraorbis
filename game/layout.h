/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LAYOUT_ALTERA_INCLUDED
#define LAYOUT_ALTERA_INCLUDED

static const int LAYOUT_VIRTUAL_HEIGHT = 600;

#if 0
// TABLET sizes. Need tuning.
static const float LAYOUT_SQUARE = 50.0f;
static const float LAYOUT_SIZE_X = 80.0f;
static const float LAYOUT_SIZE_Y = 50.0f;
static const float TEXT_HEIGHT   = 18.0f;
#else
// Laptop/Desktop
static const float LAYOUT_SQUARE = 35.0f;
static const float LAYOUT_SIZE_X = 70.0f;
static const float LAYOUT_SIZE_Y = LAYOUT_SQUARE;
static const float TEXT_HEIGHT   = 14.0f;
static const float LAYOUT_SPACING = 4.0f;
static const float LAYOUT_GUTTER =  8.0f;

#endif

// Digital bars are all kinds of layout execeptions:
static const float LAYOUT_BAR_HEIGHT_FRACTION = 0.75f;
static const float LAYOUT_BAR_ALPHA = 0.80f;
static const int   LAYOUT_TEXT_HEIGHT = 13;			// Text height in virtual pixels.

#endif // LAYOUT_ALTERA_INCLUDED