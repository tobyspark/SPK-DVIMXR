// *spark audio-visual
// OLED display using SSD1305 driver
// Copyright *spark audio-visual 2012

#ifndef SPK_OLED_GFX_h
#define SPK_OLED_GFX_h

#include "mbed.h"

const uint8_t spkDisplayLogo[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x3F, 0x1F, 0x8F, 0x07, 0x03, 0x03, 0x01, 0xC0, 0xF8, 0xBC, 0x80, 0x80, 0x80, 0x80, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xFE, 0xE0, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0x7F, 0x3F, 0x1F, 0x1F, 0x4F, 0xE7, 0xE7, 0xF3, 0xB9, 0xD8, 0xEC, 0x6E, 0x7E, 0x7D, 0x53, 0x47, 0x1E, 0x3F, 0x93, 0x83, 0xC3, 0xC1, 0xE1, 0xE1, 0xF1, 0xF9, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x80, 0xFE, 0x80, 0xF0, 0xF0, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xB3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0x7C, 0x7C, 0x3F, 0x9F, 0x0F, 0xCF, 0xE7, 0xE3, 0x33, 0xB9, 0xDC, 0x6C, 0x76, 0x3E, 0x3C, 0x7C, 0x6E, 0x05, 0x03, 0x8F, 0x8F, 0xC8, 0xE0, 0xF0, 0xF8, 0xFC, 0xFC, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF, 0x7F, 0x7F, 0x3F, 0x3F, 0x1F, 0x1F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x30, 0x60, 0x70, 0xE0, 0xE2, 0xC4, 0xCC, 0xD8, 0xF0, 0xE0, 0xC8, 0xF0, 0xE0, 0xC3, 0x8C, 0xB8, 0xF0, 0xC0, 0xC0, 0xC0, 0x80, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x3E, 0x30, 0x98, 0xCC, 0x0E, 0x27, 0x73, 0xF1, 0xD9, 0xFC, 0x6E, 0x36, 0x3B, 0x9F, 0x8F, 0xCC, 0xE1, 0xF3, 0xE7, 0xCE, 0xC8, 0xE0, 0xF6, 0xFE, 0xFE, 0xFC, 0xFE, 0xFE, 0x7F, 0x7F, 0x3F, 0x3F, 0x1F, 0x0F, 0x0F, 0x07, 0x07, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x09, 0x9B, 0xDF, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x3F, 0x3D, 0x3F, 0x7F, 0x1F, 0x87, 0xEF, 0xC7, 0x07, 0x03, 0x39, 0x79, 0xFC, 0x6E, 0x36, 0xBB, 0xDD, 0x6C, 0x76, 0x3E, 0x9C, 0x90, 0xC1, 0xE3, 0xE6, 0xD0, 0xD1, 0xCF, 0xCF, 0xCF, 0x87, 0xC7, 0x43, 0x43, 0x01, 0x01, 0x80, 0x81, 0x81, 0x81, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x81, 0x81, 0xC7, 0xD7, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0x78, 0x30, 0x83, 0xC7, 0xEF, 0x7E, 0x7E, 0x7F, 0xFF, 0xFF, 0x9C, 0x1C, 0x4C, 0xCC, 0xC0, 0xC2, 0xF3, 0xF9, 0xFC, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0x9F, 0x1B, 0x19, 0x19, 0x19, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0xA0, 0xB0, 0xF0, 0x78, 0xF9, 0xBD, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0x79, 0xFC, 0xFC, 0xFE, 0xFC, 0xF1, 0xF3, 0xF9, 0xF8, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF1, 0xF1, 0xD3, 0xC3, 0x87, 0x07, 0x07, 0x07, 0x07, 0x0F, 0x07, 0x0C, 0x0D, 0x19, 0x10, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40, 0x22, 0x71, 0x71, 0x39, 0x1C, 0x1E, 0x0F, 0x0F, 0x07, 0x03, 0x03, 0x21, 0x31, 0x18, 0x0F, 0x07, 0x03, 0x07, 0x07, 0x03, 0x00, 0x00, 0x00, 0x07, 0xFF, 0x3F, 0x0F, 0x3F, 0x3F, 0x1F, 0x0F, 0x3F, 0x7F, 0xFF, 0x7F, 0x4F, 0x1B, 0x13, 0x01, 0x03, 0x06, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF
};

// number of columns, column0 hex, column1 hex...
const uint8_t char33[] = {1, 0x2F};
const uint8_t char34[] = {1, 0x06};
const uint8_t char35[] = {5, 0x14, 0x3E, 0x14, 0x3E, 0x14};
const uint8_t char36[] = {5, 0x24, 0x2A, 0x7F, 0x2A, 0x12};
const uint8_t char37[] = {5, 0x06, 0x36, 0x08, 0x36, 0x30};
const uint8_t char38[] = {6, 0x14, 0x2A, 0x2A, 0x24, 0x10, 0x08};
const uint8_t char39[] = {1, 0x06};
const uint8_t char40[] = {2, 0x3E, 0x41};
const uint8_t char41[] = {2, 0x41, 0x3E};
const uint8_t char42[] = {5, 0x14, 0x08, 0x3E, 0x08, 0x14};
const uint8_t char43[] = {5, 0x08, 0x08, 0x3E, 0x08, 0x08};
const uint8_t char44[] = {2, 0x40, 0x20};
const uint8_t char45[] = {3, 0x08, 0x08, 0x08};
const uint8_t char46[] = {1, 0x20};
const uint8_t char47[] = {3, 0x30, 0x08, 0x06};
const uint8_t char48[] = {5, 0x1C, 0x22, 0x2A, 0x22, 0x1C};
const uint8_t char49[] = {3, 0x22, 0x3E, 0x20};
const uint8_t char50[] = {5, 0x32, 0x2A, 0x2A, 0x2A, 0x24};
const uint8_t char51[] = {5, 0x22, 0x2A, 0x2A, 0x2A, 0x14};
const uint8_t char52[] = {5, 0x18, 0x14, 0x12, 0x3E, 0x10};
const uint8_t char53[] = {5, 0x2E, 0x2A, 0x2A, 0x2A, 0x12};
const uint8_t char54[] = {5, 0x1C, 0x2A, 0x2A, 0x2A, 0x10};
const uint8_t char55[] = {4, 0x02, 0x32, 0x0A, 0x06};
const uint8_t char56[] = {5, 0x14, 0x2A, 0x2A, 0x2A, 0x14};
const uint8_t char57[] = {5, 0x04, 0x2A, 0x2A, 0x2A, 0x1C};
const uint8_t char58[] = {1, 0x24};
const uint8_t char59[] = {2, 0x40, 0x24};
const uint8_t char60[] = {3, 0x08, 0x14, 0x22};
const uint8_t char61[] = {4, 0x14, 0x14, 0x14, 0x14};
const uint8_t char62[] = {3, 0x22, 0x14, 0x08};
const uint8_t char63[] = {4, 0x01, 0x2D, 0x05, 0x02};
const uint8_t char64[] = {6, 0x3E, 0x41, 0x4D, 0x55, 0x15, 0x1E};
const uint8_t char65[] = {5, 0x3C, 0x0A, 0x0A, 0x0A, 0x3C};
const uint8_t char66[] = {5, 0x3E, 0x2A, 0x2A, 0x2A, 0x14};
const uint8_t char67[] = {5, 0x1C, 0x22, 0x22, 0x22, 0x14};
const uint8_t char68[] = {5, 0x3E, 0x22, 0x22, 0x22, 0x1C};
const uint8_t char69[] = {4, 0x3E, 0x2A, 0x2A, 0x22};
const uint8_t char70[] = {4, 0x3E, 0x0A, 0x0A, 0x02};
const uint8_t char71[] = {5, 0x1C, 0x22, 0x22, 0x2A, 0x1A};
const uint8_t char72[] = {5, 0x3E, 0x08, 0x08, 0x08, 0x3E};
const uint8_t char73[] = {1, 0x3E};
const uint8_t char74[] = {5, 0x10, 0x20, 0x20, 0x20, 0x1E};
const uint8_t char75[] = {5, 0x3E, 0x08, 0x08, 0x14, 0x22};
const uint8_t char76[] = {4, 0x3E, 0x20, 0x20, 0x20};
const uint8_t char77[] = {5, 0x3E, 0x04, 0x08, 0x04, 0x3E};
const uint8_t char78[] = {5, 0x3E, 0x04, 0x08, 0x10, 0x3E};
const uint8_t char79[] = {5, 0x1C, 0x22, 0x22, 0x22, 0x1C};
const uint8_t char80[] = {5, 0x3E, 0x0A, 0x0A, 0x0A, 0x04};
const uint8_t char81[] = {3, 0x1C, 0x22, 0x72};
const uint8_t char82[] = {5, 0x3E, 0x0A, 0x0A, 0x1A, 0x24};
const uint8_t char83[] = {5, 0x24, 0x2A, 0x2A, 0x2A, 0x12};
const uint8_t char84[] = {5, 0x02, 0x02, 0x3E, 0x02, 0x02};
const uint8_t char85[] = {5, 0x1E, 0x20, 0x20, 0x20, 0x1E};
const uint8_t char86[] = {5, 0x06, 0x18, 0x20, 0x18, 0x06};
const uint8_t char87[] = {7, 0x1E, 0x20, 0x10, 0x0E, 0x10, 0x20, 0x1E};
const uint8_t char88[] = {5, 0x22, 0x14, 0x08, 0x14, 0x22};
const uint8_t char89[] = {5, 0x02, 0x04, 0x38, 0x04, 0x02};
const uint8_t char90[] = {5, 0x22, 0x32, 0x2A, 0x26, 0x22};
const uint8_t char91[] = {2, 0x7F, 0x41};
const uint8_t char92[] = {3, 0x06, 0x08, 0x30};
const uint8_t char93[] = {2, 0x41, 0x7F};
const uint8_t char94[] = {3, 0x04, 0x02, 0x04};
const uint8_t char95[] = {4, 0x40, 0x40, 0x40, 0x40};
const uint8_t char96[] = {2, 0x02, 0x04};
const uint8_t char97[] = {5, 0x3C, 0x0A, 0x0A, 0x0A, 0x3C};
const uint8_t char98[] = {5, 0x3E, 0x2A, 0x2A, 0x2A, 0x14};
const uint8_t char99[] = {5, 0x1C, 0x22, 0x22, 0x22, 0x14};
const uint8_t char100[] = {5, 0x3E, 0x22, 0x22, 0x22, 0x1C};
const uint8_t char101[] = {4, 0x1C, 0x2A, 0x2A, 0x22};
const uint8_t char102[] = {4, 0x3E, 0x0A, 0x0A, 0x02};
const uint8_t char103[] = {5, 0x1C, 0xA2, 0xA2, 0xAA, 0x7A};
const uint8_t char104[] = {5, 0x3E, 0x08, 0x08, 0x08, 0x3E};
const uint8_t char105[] = {1, 0x3A};
const uint8_t char106[] = {5, 0x10, 0x20, 0x22, 0x22, 0x1E};
const uint8_t char107[] = {5, 0x3E, 0x08, 0x08, 0x14, 0x22};
const uint8_t char108[] = {4, 0x3E, 0x20, 0x20, 0x20};
const uint8_t char109[] = {5, 0x3E, 0x04, 0x08, 0x04, 0x3E};
const uint8_t char110[] = {5, 0x3E, 0x04, 0x08, 0x10, 0x3E};
const uint8_t char111[] = {5, 0x1C, 0x22, 0x22, 0x22, 0x1C};
const uint8_t char112[] = {5, 0x3E, 0x0A, 0x0A, 0x0A, 0x04};
const uint8_t char113[] = {3, 0x1C, 0x22, 0x72};
const uint8_t char114[] = {5, 0x3E, 0x0A, 0x0A, 0x1A, 0x24};
const uint8_t char115[] = {5, 0x24, 0x2A, 0x2A, 0x2A, 0x12};
const uint8_t char116[] = {5, 0x02, 0x02, 0x3E, 0x02, 0x02};
const uint8_t char117[] = {5, 0x1E, 0x20, 0x20, 0x20, 0x1E};
const uint8_t char118[] = {5, 0x06, 0x18, 0x20, 0x18, 0x06};
const uint8_t char119[] = {7, 0x1E, 0x20, 0x10, 0x0E, 0x10, 0x20, 0x1E};
const uint8_t char120[] = {5, 0x22, 0x14, 0x08, 0x14, 0x22};
const uint8_t char121[] = {5, 0x02, 0x04, 0x38, 0x04, 0x02};
const uint8_t char122[] = {5, 0x22, 0x32, 0x2A, 0x26, 0x22};
const uint8_t char123[] = {3, 0x08, 0x77, 0x41};
const uint8_t char124[] = {1, 0x3E};
const uint8_t char125[] = {3, 0x41, 0x77, 0x08};
const uint8_t char126[] = {4, 0x04, 0x02, 0x04, 0x02};

const int characterBytesStartChar = 33;
const int characterBytesEndChar = 126;
const uint8_t* characterBytes[] = {char33, char34, char35, char36, char37, char38, char39, char40, char41, char42, char43, char44, char45, char46, char47, char48, char49, char50, char51, char52, char53, char54, char55, char56, char57, char58, char59, char60, char61, char62, char63, char64, char65, char66, char67, char68, char69, char70, char71, char72, char73, char74, char75, char76, char77, char78, char79, char80, char81, char82, char83, char84, char85, char86, char87, char88, char89, char90, char91, char92, char93, char94, char95, char96, char97, char98, char99, char100, char101, char102, char103, char104, char105, char106, char107, char108, char109, char110, char111, char112, char113, char114, char115, char116, char117, char118, char119, char120, char121, char122, char123, char124, char125, char126, };

#endif