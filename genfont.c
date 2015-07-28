// Copyright 2007-2015 Akop Karapetyan
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>

int main(int argc, char* argv)
{
  char line[256];
  int widths[256];
  int i,j,c;

  int ascent = 0;
  int height = 0;
  int ascii = 0;
  int chars = 0;

  scanf("height %i\n", &height);
  scanf("ascent %i\n", &ascent);

  printf("unsigned short _ch[][%i] = {\n", height);

  while (!feof(stdin))
  {
    if (scanf("\nchar %d\n", &ascii) < 1 || chars > 255)
      break;

    scanf("width %d\n", &widths[chars]);

    printf("  { ");

    for (i=0;i<height;i++)
    {
      scanf("%s", line);
      c=0;

      for (j=widths[chars];j>=0;j--)
        if (line[j]=='1')
          c|=1<<(widths[chars]-j-1);

      printf("0x%04x%s", c, (i<height-1) ? "," : "");
    }

    printf(" }, /* %i */\n", chars);
    chars++;
  }

  printf("};\n\nconst PspFont PspStockFont = \n{ %i, %i,\n  {\n", height, ascent);
  for (i = 0; i < 256; i++)
  {
    if (i % 4 == 0) printf("    ");
    printf("{ 0x%02x,_ch[0x%02x] },%s", widths[i], i, (i % 4 == 3) ? "\n" : "");
  }
  printf("  }\n};\n");

  return 0;
}
