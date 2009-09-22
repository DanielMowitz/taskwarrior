////////////////////////////////////////////////////////////////////////////////
// task - a command line task list manager.
//
// Copyright 2006 - 2009, Paul Beckingham.
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the
//
//     Free Software Foundation, Inc.,
//     51 Franklin Street, Fifth Floor,
//     Boston, MA
//     02110-1301
//     USA
//
////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include "Color.h"
#include "text.h"

////////////////////////////////////////////////////////////////////////////////
static struct
{
  Color::color_id id;
  int string_id;
  std::string english_name;
  int index;                    // offset red=3 (therefore fg=33, bg=43)
} allColors[] =
{
  // Color.h enum    i18n.h              English     Index
  { Color::nocolor,  0,                  "",         0},
  { Color::black,    0/*COLOR_BLACK*/,   "black",    1}, // fg 29+0  bg 39+0
  { Color::red,      0/*COLOR_RED*/,     "red",      2},
  { Color::green,    0/*COLOR_GREEN*/,   "green",    3},
  { Color::yellow,   0/*COLOR_YELLOW*/,  "yellow",   4},
  { Color::blue,     0/*COLOR_BLUE*/,    "blue",     5},
  { Color::magenta,  0/*COLOR_MAGENTA*/, "magenta",  6},
  { Color::cyan,     0/*COLOR_CYAN*/,    "cyan",     7},
  { Color::white,    0/*COLOR_WHITE*/,   "white",    8},

};

#define NUM_COLORS (sizeof (allColors) / sizeof (allColors[0]))

////////////////////////////////////////////////////////////////////////////////
Color::Color ()
: value (COLOR_NOFG | COLOR_NOBG)
{
}

////////////////////////////////////////////////////////////////////////////////
Color::Color (const Color& other)
{
  value = other.value;
}

////////////////////////////////////////////////////////////////////////////////
Color::Color (unsigned int c)
: value (COLOR_NOFG | COLOR_NOBG)
{
  if (!(c & COLOR_FG)) value &= ~COLOR_FG;
  if (!(c & COLOR_BG)) value &= ~COLOR_BG;

  value = c & (COLOR_256 | COLOR_UNDERLINE | COLOR_BOLD | COLOR_BRIGHT |
               COLOR_BG | COLOR_FG);
}

////////////////////////////////////////////////////////////////////////////////
// Supports the following constructs:
//   [bright] [color] [on color] [bright] [underline]
//
// Where [color] is one of:
//   black
//   red
//   ...
//   grayN  0 <= N <= 23       fg 38;5;232 + N              bg 48;5;232 + N
//   greyN  0 <= N <= 23       fg 38;5;232 + N              bg 48;5;232 + N
//   colorN 0 <= N <= 255      fg 38;5;N                    bg 48;5;N
//   rgbRGB 0 <= R,G,B <= 5    fg 38;5;16 + R*36 + G*6 + B  bg 48;5;16 + R*36 + G*6 + B
Color::Color (const std::string& spec)
: value (COLOR_NOFG | COLOR_NOBG)
{
  // By converting underscores to spaces, we inherently support the old "on_red"
  // style of specifying background colors.
  std::string modifiable_spec = spec;
  std::replace (modifiable_spec.begin (), modifiable_spec.end (), '_', ' ');

  // Split spec into words.
  std::vector <std::string> words;
  split (words, modifiable_spec, ' ');

  bool bg = false;
  int index;
  std::string word;
  std::vector <std::string>::iterator it;
  for (it = words.begin (); it != words.end (); ++it)
  {
    word = lowerCase (*it);

    if (word == "bold")
    {
      value |= COLOR_BOLD;
      value &= ~COLOR_256;
    }
    else if (word == "bright")
    {
      value |= COLOR_BRIGHT;
      value &= ~COLOR_256;
    }
    else if (word == "underline")
    {
      value |= COLOR_UNDERLINE;
    }
    else if (word == "on")
    {
      bg = true;
    }

    // X where X is one of black, red, blue ...
    else if ((index = find (word)) != -1)
    {
      if (bg)
      {
        value &= ~COLOR_NOBG;
        value |= index << 8;
      }
      else
      {
        value &= ~COLOR_NOFG;
        value |= index;
      }
    }

    // greyN/grayN, where 0 <= N <= 23.
    else if (word.substr (0, 4) == "grey" ||
             word.substr (0, 4) == "gray")
    {
      index = ::atoi (word.substr (4, std::string::npos).c_str ());
      if (index < 0 || index > 23)
        throw std::string ("The color '") + *it + "' is not recognized.";

      if (bg)
      {
        value &= ~COLOR_NOBG;
        value |= (index + 232) << 8;
      }
      else
      {
        value &= ~COLOR_NOFG;
        value |= index + 232;
      }

      value |= COLOR_256;
    }

    // rgbRGB, where 0 <= R,G,B <= 5.
    else if (word.substr (0, 3) == "rgb")
    {
      index = ::atoi (word.substr (3, std::string::npos).c_str ());
      if (word.length () != 6 ||
          index < 0 || index > 555)
        throw std::string ("The color '") + *it + "' is not recognized.";

      int r = ::atoi (word.substr (3, 1).c_str ());
      int g = ::atoi (word.substr (4, 1).c_str ());
      int b = ::atoi (word.substr (5, 1).c_str ());
      if (r < 0 || r > 5 ||
          g < 0 || g > 5 ||
          b < 0 || b > 5)
        throw std::string ("The color '") + *it + "' is not recognized.";

      index = 16 + r*36 + g*6 + b;
      if (bg)
      {
        value &= ~COLOR_NOBG;
        value |= index << 8;
      }
      else
      {
        value &= ~COLOR_NOFG;
        value |= index;
      }

      value |= COLOR_256;
    }

    // colorN, where 0 <= N <= 255.
    else if (word.substr (0, 5) == "color")
    {
      index = ::atoi (word.substr (5, std::string::npos).c_str ());
      if (index < 0 || index > 255)
        throw std::string ("The color '") + *it + "' is not recognized.";

      if (bg)
      {
        value &= ~COLOR_NOBG;
        value |= index << 8;
      }
      else
      {
        value &= ~COLOR_NOFG;
        value |= index;
      }

      value |= COLOR_256;
    }
    else
      throw std::string ("The color '") + *it + "' is not recognized.";
  }
}

////////////////////////////////////////////////////////////////////////////////
Color::Color (color_id fg, color_id bg, bool underline, bool bold, bool bright)
: value (COLOR_NOFG | COLOR_NOBG)
{
  value = ((underline ? 1 : 0) << 18)
        | ((bold      ? 1 : 0) << 17)
        | ((bright    ? 1 : 0) << 16);

  if (bg != Color::nocolor)
  {
    value &= ~COLOR_NOBG;
    value |= (bg << 8);
  }

  if (fg != Color::nocolor)
  {
    value &= ~COLOR_NOFG;
    value |= fg;
  }
}

////////////////////////////////////////////////////////////////////////////////
Color::~Color ()
{
}

////////////////////////////////////////////////////////////////////////////////
Color& Color::operator= (const Color& other)
{
  if (this != &other)
    value = other.value;

  return *this;
}

////////////////////////////////////////////////////////////////////////////////
Color::operator std::string ()
{
  std::string description;
  if (value & COLOR_BOLD) description += "bold";

  if (value & COLOR_UNDERLINE)
    description += std::string (description.length () ? " " : "") + "underline";

  if (!(value & COLOR_NOFG))
    description += std::string (description.length () ? " " : "") + fg ();

  if (!(value & COLOR_NOBG))
  {
    description += std::string (description.length () ? " " : "") + "on";

    if (value & COLOR_BRIGHT)
      description += std::string (description.length () ? " " : "") + "bright";

    description += " " + bg ();
  }

  return description;
}

////////////////////////////////////////////////////////////////////////////////
Color::operator int ()
{
  return (int) value;
}

////////////////////////////////////////////////////////////////////////////////
// If 'other' has styles that are compatible, merge them into this.  Colors in
// other overwrite.
void Color::blend (const Color& other)
{
  // Matching 256-color specifications.  Merge all relevant bits.
  if (value       & COLOR_256 &&
      other.value & COLOR_256)
  {
    if (!(other.value & COLOR_NOBG))
    {
      value &= ~COLOR_BG;                      // Remove previous color.
      value |= (other.value & COLOR_BG);       // Apply new color.
      value &= ~COLOR_NOBG;                    // Now have a color.
    }

    if (!(other.value & COLOR_NOFG))
    {
      value &= ~COLOR_FG;                      // Remove previous color.
      value |= (other.value & COLOR_FG);       // Apply new color.
      value &= ~COLOR_NOFG;                    // Now have a color.
    }
  }

  // Matching 16-color specifications.  Merge all relevant bits.
  else if (!(value       & COLOR_256) &&
           !(other.value & COLOR_256))
  {
    value |= (other.value & COLOR_BOLD);       // Inherit boldness.
    value |= (other.value & COLOR_BRIGHT);     // Inherit brightness.

    if (!(other.value & COLOR_NOBG))
    {
      value &= ~COLOR_BG;                      // Remove previous color.
      value |= (other.value & COLOR_BG);       // Apply new color.
      value &= ~COLOR_NOBG;                    // Now have a color.
    }

    if (!(other.value & COLOR_NOFG))
    {
      value &= ~COLOR_FG;                      // Remove previous color.
      value |= (other.value & COLOR_FG);       // Apply new color.
      value &= ~COLOR_NOFG;                    // Now have a color.
    }
  }

  // If a 16-color is blended with a 256-color, then the 16-color is upgraded.
  else if (!(value & COLOR_256) &&
           other.value & COLOR_256)
  {
    value |= COLOR_256;                        // Upgrade to 256-color.
    value &= ~COLOR_BOLD;                      // Ignore boldness.
    value &= ~COLOR_BRIGHT;                    // Ignore brightness.
    value &= ~COLOR_FG;                        // Ignore original 16-color.
    value &= ~COLOR_BG;                        // Ignore original 16-color.
    value |= COLOR_NOFG;                       // No fg.
    value |= COLOR_NOBG;                       // No bg.

    if (!(other.value & COLOR_NOBG))
    {
      value &= ~COLOR_BG;                      // Remove previous color.
      value |= (other.value & COLOR_BG);       // Apply new color.
      value &= ~COLOR_NOBG;                    // Now have a color.
    }

    if (!(other.value & COLOR_NOFG))
    {
      value &= ~COLOR_FG;                      // Remove previous color.
      value |= (other.value & COLOR_FG);       // Apply new color.
      value &= ~COLOR_NOFG;                    // Now have a color.
    }
  }

  value |= (other.value & COLOR_UNDERLINE);    // Always inherit underline.
}

////////////////////////////////////////////////////////////////////////////////
/*
  red                  \033[31m
  bold red             \033[91m
  underline red        \033[4;31m
  bold underline red   \033[1;4;31m

  on red               \033[41m
  on bright red        \033[101m

  256 fg               \033[38;5;Nm
  256 bg               \033[48;5;Nm
*/
std::string Color::colorize (const std::string& input)
{
  if (value == 0)
    return input;

  int count = 0;
  std::stringstream result;

  // 256 color
  if (value & COLOR_256)
  {
    if (value & COLOR_UNDERLINE)
      result << "\033[4m";

    if (!(value & COLOR_NOFG))
      result << "\033[38;5;" << (value & COLOR_FG) << "m";

    if (!(value & COLOR_NOBG))
      result << "\033[48;5;" << ((value & COLOR_BG) >> 8) << "m";
  }

  // 16 color
  else
  {
    result << "\033[";

    if (value & COLOR_BOLD)
    {
      if (count++) result << ";";
      result << "1";
    }

    if (value & COLOR_UNDERLINE)
    {
      if (count++) result << ";";
      result << "4";
    }

    if (!(value & COLOR_NOBG))
    {
      if (count++) result << ";";
      result << ((value & COLOR_BRIGHT ? 99 : 39) + ((value & COLOR_BG) >> 8));
    }

    if (!(value & COLOR_NOFG))
    {
      if (count++) result << ";";
      result << (29 + (value & COLOR_FG));
    }

    result << "m";
  }

  result << input << "\033[0m";
  return result.str ();
}

////////////////////////////////////////////////////////////////////////////////
std::string Color::colorize (const std::string& input, const std::string& spec)
{
  Color c (spec);
  return c.colorize (input);
}

////////////////////////////////////////////////////////////////////////////////
int Color::find (const std::string& input)
{
  for (unsigned int i = 0; i < NUM_COLORS; ++i)
    if (allColors[i].english_name == input)
      return (int) i;

  return -1;
}

////////////////////////////////////////////////////////////////////////////////
std::string Color::fg ()
{
  int index = value & COLOR_FG;

  if (value & COLOR_256)
  {
    if (!(value & COLOR_NOFG))
    {
      std::stringstream s;
      s << "color" << (value & COLOR_FG);
      return s.str ();
    }
  }
  else
  {
    for (unsigned int i = 0; i < NUM_COLORS; ++i)
      if (allColors[i].index == index)
        return allColors[i].english_name;
  }

  return "";
}

////////////////////////////////////////////////////////////////////////////////
std::string Color::bg ()
{
  int index = (value & COLOR_BG) >> 8;

  if (value & COLOR_256)
  {
    if (!(value & COLOR_NOBG))
    {
      std::stringstream s;
      s << "color" << ((value & COLOR_BG) >> 8);
      return s.str ();
    }
  }
  else
  {
    for (unsigned int i = 0; i < NUM_COLORS; ++i)
      if (allColors[i].index == index)
        return allColors[i].english_name;
  }

  return "";
}

////////////////////////////////////////////////////////////////////////////////
