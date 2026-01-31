# SSD1306 OLED Display Library for STM32

Complete library for driving SSD1306-based OLED displays (128x64 pixels) via I2C on STM32 microcontrollers.

## Features

- **Multiple Font Sizes**: Font_7x10 (fully working), Font_20x32 (placeholder for custom large font)
- **Graphics Functions**: Lines, rectangles, circles, bitmaps
- **Text Rendering**: Multi-line text support with `\n`
- **Display Control**: Contrast, inversion, scrolling
- **Hardware Scrolling**: Left/right smooth scrolling
- **Full Drawing Buffer**: Update entire screen at once

## Available Fonts

### ✅ Font_7x10 - Fully Implemented
- **Size**: 7 × 10 pixels
- **Characters**: Full ASCII (32-126)
- **Memory**: ~665 bytes
- **Use**: General text, labels, status messages

### ✅ Font_21x32 - Yu Gothic Large Font
- **Size**: 21 × 32 pixels  
- **Characters**: space, 0-9, B, P (13 characters)
- **Memory**: ~1248 bytes
- **Use**: Large "Bxx Px" display format (e.g., "B42 P7")
- **Status**: ✅ Fully working - easily replaceable with other fonts

### 🚧 Font_20x32 - Placeholder
- **Size**: 20 × 32 pixels  
- **Characters**: Not implemented
- **Status**: Placeholder for future use

---

## Quick Guide: Using LVGL Font Converter (Recommended) ✨

LVGL Font Converter is easier and more reliable than TheDotFactory for this project.

### Step-by-Step Process

**1. Open LVGL Font Converter**
   - URL: https://lvgl.io/tools/fontconverter
   - No installation needed - works in browser

**2. Configure Font**
   - **Name**: Font_21x32 (or any name you like)
   - **Size**: 42-44 px (this will give you ~32px actual height)
   - **Bpp**: 1 bit-per-pixel ⚠️
   - **TTF/WOFF font**: Upload your font file (e.g., `C:\Windows\Fonts\segoeuib.ttf`)
   - Or use built-in fonts from the dropdown
   
   **Note**: LVGL's "size" is font point size, not pixel height. 
   For 32-pixel tall characters, use size 42-44px and check the preview.

**3. Set Character Range**
   - **Range**: Custom range in list format
   - Enter: `0x20,0x30-0x39,0x42,0x50` 
   - (This is: space, digits 0-9, letter B, letter P)

**4. Configure Output**
   - **Bpp**: 1 bit-per-pixel ⚠️
   - Try checking "Compressed" for smaller size

**5. Generate and Download**
   - Click "Convert"
   - Download the generated `.c` file
   
**6. Extract Bitmap Data**
   - Open the downloaded file in text editor
   - Find the bitmap array (looks like `static const uint8_t glyph_bitmap[] = {`)
   - LVGL format is typically column-major with MSB first - we'll adapt it

**7. Next Steps**
   - Copy the bitmap data
   - We'll integrate it into the Font_21x32 structure
   - I'll help you adapt the format if needed

---

## Quick Guide: Using TheDotFactory (Alternative - More Complex)

### Step-by-Step Process

**1. Configure TheDotFactory**
   - Download: https://github.com/pavius/the-dot-factory
   - Run `TheDotFactory.exe`
   - Try different TrueType fonts to see what looks best!
   - Set **Height**: 32 pixels
   - ⚠️ **Width**: Fixed width (NOT Proportional!)
   - Set fixed width value to **24 pixels** (safe maximum)
   
   **Why Fixed Width?**  
   Proportional width generates different bytes-per-row for each character, causing  
   alignment issues. Fixed width ensures all characters use the same byte structure.

**2. Output Settings**
   - **Bit layout**: Row Major ⚠️
   - **Bit numbering**: LSB First ⚠️ (NOT MSB First!)
   - **Byte format**: Hex
   - **Comment style**: C-style with visual

**3. Character Selection**
   - **Range**: Custom
   - Enter: `0123456789BP` (no space - we'll add it manually)

**4. Generate and Copy**
   - Click "Generate Font"
   - Copy ALL bitmap data from the output
   - From `// @0 '0'` to the last character `'P'`
   - Don't copy array declarations or FONT_INFO structures

**5. Update ssd1306_fonts.c**
   - Open `Lib/SSD1306/ssd1306_fonts.c`
   - Find the `Font21x32_data[]` array (~line 306)
   - **Keep** the space character at the top (lines with `// @0 ' '`)
   - **Replace** everything from `// @96 '0'` onwards with your data
   - **Change offset**: Edit your first character to `// @96 '0'` (after 96-byte space)

**6. Adjust Width**
   - Look at your generated data - find the character comment that shows width
   - Example: `// @96 '0' (24 pixels wide)` means width = 24
   - Update two places:
     - In `ssd1306_fonts.c`: `FontDef Font_21x32 = {24, 32, ...` (change first number)
     - In `ssd1306.c` (~line 161): `if (Font.width == 24 && Font.height == 32)`

**7. Build, Flash, Test!**
   ```c
   SSD1306_WriteString("B42 P7", Font_21x32, White);
   ```

**Popular fonts to try:**
- Arial Bold, Roboto Bold, Helvetica Bold
- Consolas Bold (monospace), Impact
- Segoe UI Bold, Calibri Bold

---

## Advanced: Adding Completely New Fonts (Different Characters)

If you want different characters (not just 0-9, B, P):

### Method 1: LVGL Font Converter (Online) ⭐

1. **URL**: https://lvgl.io/tools/fontconverter (✅ Currently working, no download needed)

2. **Upload Font**
   - Click "Browse" and select a TrueType font (or use built-in fonts)
   - Windows fonts location: `C:\Windows\Fonts\arialbd.ttf` (Arial Bold)

3. **Configure Settings**
   - **Name**: Font20x32
   - **Size**: 32 px
   - **Bpp**: 1 bit-per-pixel
   - **Characters**: `0123456789BP ` (include space)
   - **Range Format**: Text
   - Try "Compressed" for smaller size

4. **Generate & Download**
   - Click "Convert"
   - Download the generated C file
   - Extract bitmap data and adapt to column-major format
   - Paste into `ssd1306_fonts.c`

### Method 2: TheDotFactory (Windows Application)

1. **Download** (Multiple sources, as original site may be down)
   - GitHub: https://github.com/pavius/the-dot-factory
   - Alternative: Search "TheDotFactory font generator download"
   - Extract and run TheDotFactory.exe

2. **Load Font**
   - Select a bold TrueType font (Arial Bold, Roboto Bold)
   - Set **Height**: 32 pixels
   - Set **Width**: Fixed 20 pixels

3. **Configure Output**
   - **Bit Layout**: Column Major
   - **Bit Numbering**: LSB First
   - **Format**: C Array

4. **Select Characters**
   - **Range**: Custom
   - Enter: `0123456789BP ` (note space at end)

5. **Generate & Export**
   - Click Generate
   - Copy C array → Paste into `ssd1306_fonts.c`

### Method 3: Bitmap2LCD (Trial Version)

1. **URL**: http://www.bitmap2lcd.com/
2. Professional tool with free trial
3. Good for complex fonts and graphics

### Method 4: Python Script (For Programmers)

If you have Python with PIL/Pillow installed:

```python
from PIL import Image, ImageDraw, ImageFont
import os

def generate_char(char, font_path, size=32, width=20):
    """Generate column-major bitmap data for one character"""
    font = ImageFont.truetype(font_path, size)
    img = Image.new('1', (width, size), 0)
    draw = ImageDraw.Draw(img)
    
    # Center character
    bbox = draw.textbbox((0, 0), char, font=font)
    x_offset = (width - (bbox[2] - bbox[0])) // 2
    draw.text((x_offset, -bbox[1]), char, font=font, fill=1)
    
    # Convert to column-major bytes (LSB first)
    data = []
    for col in range(width):
        for byte_idx in range(4):  # 32 pixels = 4 bytes
            byte_val = 0
            for bit in range(8):
                row = byte_idx * 8 + bit
                if row < size and img.getpixel((col, row)):
                    byte_val |= (1 << bit)
            data.append(f"0x{byte_val:02X}")
    return ",".join(data)

# Generate all characters
font_path = "C:/Windows/Fonts/arialbd.ttf"  # Windows
# font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"  # Linux

if os.path.exists(font_path):
    for ch in " 0123456789BP":
        char_name = "space" if ch == " " else ch
        print(f"    // Character '{char_name}'")
        print(f"    {generate_char(ch, font_path)},")
else:
    print(f"Font not found: {font_path}")
```

**To run:**
```bash
pip install pillow
python font_generator.py > font_data.txt
```

---

## API Usage

### Initialization
```c
#include "ssd1306.h"

// After I2C peripheral initialization
SSD1306_Init(&hi2c1);
```

### Text Display
```c
SSD1306_Fill(Black);
SSD1306_SetCursor(10, 5);
SSD1306_WriteString("Hello", Font_7x10, White);
SSD1306_UpdateScreen();
```

### Large Custom Display (when Font_20x32 data added)
```c
SSD1306_Fill(Black);
SSD1306_SetCursor(4, 16);  // Centered for "B42 P7"
SSD1306_WriteString("B42 P7", Font_20x32, White);
SSD1306_UpdateScreen();
```

### Graphics
```c
SSD1306_DrawLine(x0, y0, x1, y1, White);
SSD1306_DrawRectangle(x, y, width, height, White);
SSD1306_DrawCircle(x, y, radius, White);
SSD1306_UpdateScreen();
```

### Display Control
```c
SSD1306_Fill(Black);              // Clear screen
SSD1306_SetContrast(128);         // Brightness 0-255
SSD1306_InvertDisplay(true);      // Invert colors
SSD1306_ScrollRight(0, 7);        // Scroll animation
SSD1306_StopScroll();             // Stop scrolling
```

---

## Font Data Format

Font_20x32 uses **column-major** storage:

```
Character dimensions: 20 columns × 32 rows
Bytes per column: 32 pixels ÷ 8 = 4 bytes
Total per character: 20 × 4 = 80 bytes

Column byte layout (LSB first):
  Byte 0: Rows 0-7   (bit 0 = row 0, bit 7 = row 7)
  Byte 1: Rows 8-15
  Byte 2: Rows 16-23
  Byte 3: Rows 24-31

Total font size: 13 characters × 80 bytes = 1040 bytes
```

---

## Centering "Bxx Px" on 128x64 Display

```c
// "B42 P7" = 6 characters × 20 pixels = 120 pixels wide
// Horizontal center: (128 - 120) / 2 = 4
// Vertical center: (64 - 32) / 2 = 16

SSD1306_SetCursor(4, 16);
SSD1306_WriteString("B42 P7", Font_20x32, White);
```

---

## Troubleshooting Font Generation

| Problem | Cause | Solution |
|---------|-------|----------|
| Characters rotated/garbled | Wrong bit layout | Toggle Column/Row Major |
| Characters upside down | Wrong bit order | Toggle LSB/MSB First |
| Characters too wide | Width mismatch | Adjust width or use proportional |
| Missing characters | Incomplete range | Include all: `0123456789BP ` |

---

## Testing New Font Data

```c
// Test individual characters
SSD1306_Fill(Black);
SSD1306_SetCursor(0, 0);
SSD1306_WriteChar('0', Font_20x32, White);
SSD1306_UpdateScreen();
HAL_Delay(1000);

SSD1306_Fill(Black);
SSD1306_WriteChar('B', Font_20x32, White);
SSD1306_UpdateScreen();
```

---

## I2C Configuration

- **Address**: `0x78` (0x3C << 1)
- **Speed**: 400 kHz (Fast Mode) recommended
- **Pins**: Configurable via CubeMX (typically PB6=SCL, PB7=SDA)

If your display uses a different address (some use 0x7A), change `SSD1306_I2C_ADDR` in [ssd1306.h](Lib/SSD1306/ssd1306.h).

## Example Code

The example in [main.c](Core/Src/main.c) demonstrates:
- Displaying text with different fonts
- Drawing lines and circles
- Basic screen layout

Build and flash to see it working!
