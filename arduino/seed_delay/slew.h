/*

    mplsartindustry/daisy
    Copyright (c) 2024-2025 held jointly by the individual authors.

    This file is part of mplsartindustry/daisy.

    mplsartindustry/daisy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    mplsartindustry/daisy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with mplsartindustry/daisy.  If not, see <http://www.gnu.org/licenses/>.

*/

class Slew
{
public:
  double last;
  double c;
  double noiseFloor;
  int settleSamples = 0;
  int settleSamplesThreshold = 96;

  void Init(double c = 0.001, double noiseFloor = 0.0)
  {
    this->last = 0.0f;
    this->c = c;
    this->noiseFloor = noiseFloor;
  }

  float Process(float x)
  {
    double d = (x - last);
    if (noiseFloor > 0.0f)
    {
      if (abs(d) < noiseFloor)
      {
        if (settleSamples < settleSamplesThreshold)
        {
          settleSamples++;
        }
        else
        {
          d = 0.0f;
        }
      }
      else
      {
        settleSamples = 0;
      }
    }
    last = last + d * c;
    return last;
  }
};
