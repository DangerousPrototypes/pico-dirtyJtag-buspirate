#include <stdio.h>
#include "pico/stdlib.h"
#include "../pirate.h"
#include "../shift.h"
#include "hardware/spi.h"
#include "font/font.h" 
#include "font/hunter-23pt-24h24w.h"
//#include "font/hunter-20pt-21h21w.h"
#include "font/hunter-14pt-19h15w.h"
//#include "font/hunter-12pt-16h13w.h" 
#include "background.h"
#include "background_image_v4.h"
#include "display.h"

void lcd_write_string(
    const FONT_INFO* font, const uint8_t* back_color, const uint8_t* text_color, const char* c, uint16_t fill_length);
void lcd_write_labels(uint16_t left_margin,
                      uint16_t top_margin,
                      const FONT_INFO* font,
                      const uint8_t* color,
                      const char* c,
                      uint16_t fill_length);

const uint8_t colors_pallet[][2] = {
    [LCD_RED] = { (uint8_t)(BP_LCD_COLOR_RED >> 8), (uint8_t)BP_LCD_COLOR_RED },
    [LCD_ORANGE] = { (uint8_t)(BP_LCD_COLOR_ORANGE >> 8), (uint8_t)BP_LCD_COLOR_ORANGE },
    [LCD_YELLOW] = { (uint8_t)(BP_LCD_COLOR_YELLOW >> 8), (uint8_t)BP_LCD_COLOR_YELLOW },
    [LCD_GREEN] = { (uint8_t)(BP_LCD_COLOR_GREEN >> 8), (uint8_t)BP_LCD_COLOR_GREEN },
    [LCD_BLUE] = { (uint8_t)(BP_LCD_COLOR_BLUE >> 8), (uint8_t)BP_LCD_COLOR_BLUE },
    [LCD_PURPLE] = { (uint8_t)(BP_LCD_COLOR_PURPLE >> 8), (uint8_t)BP_LCD_COLOR_PURPLE },
    [LCD_BROWN] = { (uint8_t)(BP_LCD_COLOR_BROWN >> 8), (uint8_t)BP_LCD_COLOR_BROWN },
    [LCD_GREY] = { (uint8_t)(BP_LCD_COLOR_GREY >> 8), (uint8_t)BP_LCD_COLOR_GREY },
    [LCD_WHITE] = { (uint8_t)(BP_LCD_COLOR_WHITE >> 8), (uint8_t)BP_LCD_COLOR_WHITE },
    [LCD_BLACK] = { (uint8_t)(BP_LCD_COLOR_BLACK >> 8), (uint8_t)BP_LCD_COLOR_BLACK },
};

struct display_layout {
    const struct lcd_background_image_info* image;
    const FONT_INFO* font_default;
    const FONT_INFO* font_big;
    uint8_t current_top_pad;
    uint8_t current_left_pad;
    enum lcd_colors current_color;
    uint8_t ma_top_pad;
    uint8_t ma_left_pad;
    enum lcd_colors ma_color;
    uint8_t vout_top_pad;
    uint8_t io_col_top_pad;
    uint8_t io_col_left_pad;    // column begins 7 pixels from the left
    uint8_t io_col_width_chars; // 4 character columns
    uint8_t io_col_right_pad;   // 6 pixels padding space between columns
    uint8_t io_row_height;
    enum lcd_colors io_name_color;
    enum lcd_colors io_label_color;
    enum lcd_colors io_value_color;
};

const struct display_layout layout = {
    &lcd_back_v4,
    &hunter_14ptFontInfo, // const FONT_INFO *font_default;
    &hunter_23ptFontInfo, // const FONT_INFO *font_big;
    36,                   // uint8_t current_top_pad;
    58,                   // uint8_t current_left_pad;
    LCD_RED,              // enum lcd_colors current_color;
    43,                   // uint8_t ma_top_pad;
    172,                  // uint8_t ma_left_pad;
    LCD_RED,              // enum lcd_colors ma_color;
    7,                    // uint8_t vout_top_pad;
    72,                   // uint8_t io_col_top_pad;
    7,                    // uint8_t io_col_left_pad;    //column begins 7 pixels from the left
    4,                    // uint8_t io_col_width_chars; //4 character columns
    6,                    // uint8_t io_col_right_pad; //6 pixels padding space between columns
    28,                   // uint8_t io_row_height;
    LCD_RED,              // enum lcd_colors io_name_color;
    LCD_WHITE,            // enum lcd_colors io_label_color;
    LCD_RED               // enum lcd_colors io_value_color;
};

void lcd_write_background(const unsigned char* image) {
    lcd_set_bounding_box(0, 240, 0, 320);

    //spi_busy_wait(true);
    gpio_put(DISPLAY_DP, 1);
    gpio_put(DISPLAY_CS, 0);

    // Update October 2024: new image headers in pre-sorted pixel format for speed
    //  see image.py in the display folder to create new headers
    // TODO: DMA it.
    spi_write_blocking(BP_SPI_PORT, image, (320 * 240 * 2));

    gpio_put(DISPLAY_CS, 1);
    //spi_busy_wait(false);
}

// Write a string to the LCD
// TODO: in LCD write string, automaticall toupper/lower depending on the contents of the font and the string
void lcd_write_string(
    const FONT_INFO* font, const uint8_t* back_color, const uint8_t* text_color, const char* c, uint16_t fill_length) {
    uint16_t row;
    uint16_t length = 0;
    uint8_t adjusted_c=0;

    while (*c > 0) {
        adjusted_c = (*c) - (*font).start_char;
        for (uint16_t col = 0; col < (*font).lookup[adjusted_c].width; col++) {
            row = 0;
            uint16_t rows = (*font).lookup[adjusted_c].height;
            uint16_t offset = (*font).lookup[adjusted_c].offset;

            for (uint16_t page = 0; page < (*font).height_bytes; page++) {

                uint8_t bitmap_char = (*font).bitmaps[offset + (col * (*font).height_bytes) + page];

                for (uint8_t i = 0; i < 8; i++) {
                    if (bitmap_char & (0b10000000 >> i)) {
                        spi_write_blocking(BP_SPI_PORT, text_color, 2);
                    } else {
                        spi_write_blocking(BP_SPI_PORT, back_color, 2);
                    }

                    row++;
                    // break out of loop when we have all the rows
                    // some bits may be discarded because of poor packing by The Dot Factory
                    if (row == rows) {
                        break;
                    }
                }
            }
        }
        // depending on how the font fits in the bitmap,
        // there may or may not be enough right hand padding between characters
        // this adds a configurable amount of space
        for (uint8_t pad = 0; pad < (*font).lookup[adjusted_c].height * (*font).right_padding; pad++) {
            spi_write_blocking(BP_SPI_PORT, back_color, 2);
        }
        (c)++;
        length++; // how many characters have we written
    }

    // add additional blank spaces to clear old characters off the line
    if (length < fill_length) {
        uint32_t fill = (fill_length - length) * ((*font).lookup[adjusted_c].height *
                                                  ((*font).right_padding + (*font).lookup[adjusted_c].width));
        for (uint32_t i = 0; i < fill; i++) {
            spi_write_blocking(BP_SPI_PORT, back_color, 2);
        }
    }
}

uint16_t lcd_get_col(const struct display_layout* layout, const FONT_INFO* font, uint8_t col) {
    uint16_t col_start = (*font).lookup[0].width + (*font).right_padding; // width of a character
    col_start *= layout->io_col_width_chars;
    col_start += layout->io_col_right_pad;
    col_start *= (col - 1);
    col_start += layout->io_col_left_pad;
    return col_start;
}

//modified to draw once with pin labels and then move onto the main firmware
void ui_lcd_update(const char *app_name, const char *vout_pin_label, const char *vout_func_label, const char *vout_direction, const char **hw_pin_label_ordered, const char **func_pin_label_ordered, const char **direction_pin_label_ordered) {

    lcd_write_background(layout.image->bitmap);

    // firmware title
    uint16_t left_margin = lcd_get_col(&layout, layout.font_default, 1); // put us in the first column
    uint16_t top_margin = layout.vout_top_pad;
    lcd_write_labels(left_margin,
                     top_margin,
                     layout.font_default,
                     colors_pallet[layout.io_label_color],
                     app_name,  layout.io_col_width_chars);

    // IO pin name loop
    //first is vout_pin_label
    top_margin = layout.io_col_top_pad-layout.io_row_height;
    lcd_write_labels(left_margin,
                    top_margin,
                    layout.font_default,
                    colors_pallet[layout.io_name_color],
                    vout_pin_label,
                    layout.io_col_width_chars);
    top_margin = layout.io_col_top_pad;
    //now the array of fixed pin labels
    for (int i = 1; i < HW_PINS; i++) {
        lcd_write_labels(left_margin,
                            top_margin,
                            layout.font_default,
                            colors_pallet[layout.io_name_color],
                            hw_pin_label_ordered[i-1],
                            layout.io_col_width_chars);
        // Vout gets to set own position, the next pins go in even rows
        top_margin = layout.io_col_top_pad + (layout.io_row_height * i);
    }


// labels
    left_margin = lcd_get_col(&layout, layout.font_default, 2); // put us in the second column
    // IO pin label loop
    //top_margin = layout.vout_top_pad;
    if(vout_func_label[0]){
        top_margin = layout.io_col_top_pad-layout.io_row_height;
        lcd_write_labels(left_margin,
                        top_margin,
                        layout.font_default,
                        colors_pallet[layout.io_label_color],
                        vout_func_label,
                        layout.io_col_width_chars);
    }
    // pin labels for 1...9
    top_margin = layout.io_col_top_pad;
    for (int i = 1; i < HW_PINS; i++) {
        lcd_write_labels(left_margin,
                            top_margin,
                            layout.font_default,
                            colors_pallet[layout.io_label_color],
                            func_pin_label_ordered[i-1],
                            layout.io_col_width_chars);

        // Vout gets to set own position, the next pins go in even rows
        top_margin = layout.io_col_top_pad + (layout.io_row_height * i);
    }

    //pin direction
    left_margin = lcd_get_col(&layout, layout.font_default, 3); // put us in the third column
    top_margin = layout.io_col_top_pad-layout.io_row_height;
    lcd_write_labels(left_margin,
                    top_margin,
                    layout.font_default,
                    colors_pallet[layout.io_label_color],
                    vout_direction,
                    layout.io_col_width_chars);    
    // IO voltage
    top_margin = layout.io_col_top_pad; // first line of IO pins
    for (int i = 1; i < HW_PINS - 1; i++) {
        lcd_write_labels(
            left_margin, 
            top_margin, 
            layout.font_default, 
            colors_pallet[layout.io_label_color], 
            direction_pin_label_ordered[i-1],
            0);
        top_margin += layout.io_row_height;
    }
    
}

void lcd_write_labels(uint16_t left_margin,
                      uint16_t top_margin,
                      const FONT_INFO* font,
                      const uint8_t* color,
                      const char* c,
                      uint16_t fill_length) {
    lcd_set_bounding_box(left_margin,
                         left_margin + ((240) - 1),
                         top_margin,
                         (top_margin + (*font).lookup[(*c) - (*font).start_char].height) - 1);
    //spi_busy_wait(true);
    gpio_put(DISPLAY_DP, 1);
    gpio_put(DISPLAY_CS, 0);
    lcd_write_string(font, layout.image->text_background_color, color, c, fill_length);
    gpio_put(DISPLAY_CS, 1);
    //spi_busy_wait(false);
}

void lcd_clear(void) {
    uint16_t x, y;

    lcd_set_bounding_box(0, 240, 0, 320);

    //spi_busy_wait(true);
    gpio_put(DISPLAY_DP, 1);
    gpio_put(DISPLAY_CS, 0);
    for (x = 0; x < 240; x++) {
        for (y = 0; y < 320; y++) {
            spi_write_blocking(BP_SPI_PORT, colors_pallet[LCD_BLACK], 2);
        }
    }
    gpio_put(DISPLAY_CS, 1);
    //spi_busy_wait(false);
}

void lcd_set_bounding_box(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye) {
    // setup write area
    // start must always be =< end
    lcd_write_command(0x2A); // column start and end set
    lcd_write_data(ys >> 8);
    lcd_write_data(ys & 0xff); // 0
    lcd_write_data(ye >> 8);
    lcd_write_data(ye & 0xff); // 320

    lcd_write_command(0x2B); // row start and end set

    lcd_write_data(xs >> 8);
    lcd_write_data(xs & 0xff); // 0
    lcd_write_data(xe >> 8);
    lcd_write_data(xe & 0xff); // LCD_W (240)
    lcd_write_command(0x2C);   // Memory Write
}

void lcd_write_command(uint8_t command) {
    // D/C low for command
    //spi_busy_wait(true);
    gpio_put(DISPLAY_DP, 0);                      // gpio_clear(BP_LCD_DP_PORT,BP_LCD_DP_PIN);
    gpio_put(DISPLAY_CS, 0);                      // gpio_clear(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    spi_write_blocking(BP_SPI_PORT, &command, 1); // spi_xfer(BP_LCD_SPI, (uint16_t) command);
    gpio_put(DISPLAY_CS, 1);                      // gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    //spi_busy_wait(false);
}

void lcd_write_data(uint8_t data) {
    // D/C high for data
    //spi_busy_wait(true);
    gpio_put(DISPLAY_DP, 1);                   // gpio_set(BP_LCD_DP_PORT,BP_LCD_DP_PIN);
    gpio_put(DISPLAY_CS, 0);                   // gpio_clear(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    spi_write_blocking(BP_SPI_PORT, &data, 1); // spi_xfer(BP_LCD_SPI, &data);
    gpio_put(DISPLAY_CS, 1);                   // gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    //spi_busy_wait(false);
}

void lcd_disable(void) {
    lcd_write_command(0x28);
}

void lcd_enable(void) {
    lcd_write_command(0x29);
}

void lcd_configure(void) {
    lcd_write_command(0x36);    // MADCTL (36h): Memory Data Access Control
    lcd_write_data(0b00100000); // 0x00 101/011 - left/right hand mode 0b100000

    lcd_write_command(0x3A); // COLMOD,interface pixel format
    lcd_write_data(0x55);    // 252K

    lcd_write_command(0xB2); // porch setting,, default=0C/0C/00/33/33
    lcd_write_data(0x0c);    // was 05
    lcd_write_data(0x0c);    // was 05
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);

    lcd_write_command(0xB7); // Gate Control for VGH and VGL setting, default=35
    lcd_write_data(0x70);    // was 0x75, 0x62

    lcd_write_command(0xBB); // VCOMS setting (0.1~1.675 V), default=20
    lcd_write_data(0x21);    // was 22

    lcd_write_command(0xC0); // LCM control, default=2C
    lcd_write_data(0x2C);

    lcd_write_command(0xC2); // VDV and VRH command enable, default=01 or FF
    lcd_write_data(0x01);

    lcd_write_command(0xC3); // VRH set (VRH=GVDD), default=0B
    lcd_write_data(0x0B);    // was 0x13

    lcd_write_command(0xC4); // VDV set, default=20
    lcd_write_data(0x27);    // VDV=0v //was 0x20

    lcd_write_command(0xC6); // FRCTRL=Frame Rate Control in Normal Mode , default=0F
    lcd_write_data(0x0F);    // 0x11

    lcd_write_command(0xD0); // Power Control, default=A4/81
    lcd_write_data(0xA4);    // Constant
    lcd_write_data(0xA1);    // AVDD=6.8V;AVCL=-4.8V;VDDS=2.3V

    // lcd_write_command(0xD6);
    // lcd_write_data(0xA1);

    lcd_write_command(0xE0); // PVGAMCTRL:Positive Voltage Gamma Control
    lcd_write_data(0xD0);
    lcd_write_data(0x06); // 05
    lcd_write_data(0x0B); // A
    lcd_write_data(0x09);
    lcd_write_data(0x08);
    lcd_write_data(0x30); // 05
    lcd_write_data(0x30); // 2E
    lcd_write_data(0x5B); // 44
    lcd_write_data(0x4B); // 45
    lcd_write_data(0x18); // 0f
    lcd_write_data(0x14); // 17
    lcd_write_data(0x14); // 16
    lcd_write_data(0x2C); // 2b
    lcd_write_data(0x32); // 33

    lcd_write_command(0xE1); // NVGAMCTRL:Negative Voltage Gamma Control
    lcd_write_data(0xD0);
    lcd_write_data(0x05);
    lcd_write_data(0x0A); // 0a
    lcd_write_data(0x0A); // 09
    lcd_write_data(0x07); // 08
    lcd_write_data(0x28); // 05
    lcd_write_data(0x32); // 2e
    lcd_write_data(0x2C); // 43
    lcd_write_data(0x49); // 45
    lcd_write_data(0x18); // 0f
    lcd_write_data(0x13); // 16
    lcd_write_data(0x13); // 16
    lcd_write_data(0x2C); // 2b
    lcd_write_data(0x33);

    lcd_write_command(0x21); // Display inversion ON ,default=Display inversion OF

    lcd_write_command(0x2A); // Frame rate control
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0xEF);

    lcd_write_command(0x2B); // Display function control
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x01);
    lcd_write_data(0x3F);

    lcd_write_command(0x11); // Sleep out, DC/DC converter, internal oscillator, panel scanning "enable"
    sleep_ms(120);
    lcd_write_command(0x29); // Display ON ,default= Display OFF
}