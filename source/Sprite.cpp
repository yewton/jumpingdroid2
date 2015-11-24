#include "Sprite.hpp"

namespace GbaGraphics {

u16* oam = (u16*)OBJ_BASE_ADR;
u16* pal = (u16*)OBJ_COLORS;
u16* bg_pal = (u16*)BG_PALETTE;

void setSpriteData(const short unsigned int* sprTiles, const short unsigned int* sprPal, const int len) {
    for(int i = 0; i < len; i++) {
        oam[i] = sprTiles[i];
    }
    for (int i = 0; i < 0x100; i++) {
        pal[i] = sprPal[i];
    }
}

void setBGPalette(const short unsigned int* bgPal, const int len) {
    for (int i = 0; i < len; i++) {
        bg_pal[i] = bgPal[i];
    }
}

void setBGColor(u16 col) {
    bg_pal[0] = col;
}

void setSpritePalette(const u16 num, const u8 pn) {
    OBJATTR* sp = (OBJATTR*)OAM + num;
    sp->attr2 &= 0x0fff;
    sp->attr2 |= OBJ_PALETTE(pn);
}

void initSprites()
{
    for(u8 i = 0; i < MAX_SPRITES_NUM; i++) {
        OBJATTR* sp = (OBJATTR*)OAM + i;
        sp->attr1 &= ~OBJ_X(0x01ff);
        sp->attr0 &= ~OBJ_Y(0x00ff);
        sp->attr1 |= OBJ_X(SCREEN_WIDTH);
        sp->attr0 |= OBJ_Y(SCREEN_HEIGHT);
    }
}

void Sprite::moveSprite(u16 num, s16 x, s16 y)
{
    OBJATTR* sp = (OBJATTR*)OAM + num;

    if( x < 0 ) x += 512;
    if( y < 0 ) y += 256;

    sp->attr1 &= ~OBJ_X(0x01ff);
    sp->attr0 &= ~OBJ_Y(0x00ff);
    sp->attr1 |= OBJ_X(x);
    sp->attr0 |= OBJ_Y(y);
}

void Sprite::setSpriteSize(u16 num, u16 size)
{
    OBJATTR* sp = (OBJATTR*)OAM + num;

    sp->attr1 &= ~0xc000;
    sp->attr1 |= size;
}

void Sprite::setSpriteShape(u16 num, u16 shape) {
    OBJATTR* sp = (OBJATTR*)OAM + num;

    sp->attr0 &= ~0xc000;
    sp->attr0 |= shape;
}

void Sprite::setSpriteColor(u16 num, u16 col)
{
    OBJATTR* sp = (OBJATTR*)OAM + num;

    sp->attr0 &= ~OBJ_256_COLOR;
    sp->attr0 |= col;
}

void Sprite::setSpriteCharacter(u16 num, u16 ch)
{
    OBJATTR* sp = (OBJATTR*)OAM + num;

    sp->attr2 &= ~OBJ_CHAR(0xffff);
    sp->attr2 |= OBJ_CHAR(ch);
}

void Sprite::setSpriteHFlip(u16 num, bool mode) {
    OBJATTR* sp = (OBJATTR*)OAM + num;
    if(mode) {
        sp->attr1 |= OBJ_HFLIP;
    }else {
        sp->attr1 &= ~OBJ_HFLIP;
    }
}

void Sprite::setSpriteVFlip(u16 num, bool mode) {
    OBJATTR* sp = (OBJATTR*)OAM + num;
    if (mode) {
        sp->attr1 |= OBJ_VFLIP;
    } else {
        sp->attr1 &= ~OBJ_VFLIP;
    }
}

Sprite::Sprite(const u16 n, const u16 ch, const u8 pn, SPRITE_SIZECODE sizeCode) :
    num(n), size(getSizeBySizeCode(sizeCode)),
    shape(getShapeBySizeCode(sizeCode)),
    character(ch), hFlip(false), vFlip(false),
    posX(512), posY(256), paletteNumber(pn) {
    setSpriteCharacter(num, ch);
    setSpriteSize(num, size);
    setSpriteShape(num, shape);
    setSpriteColor(num, OBJ_16_COLOR);
}

void Sprite::draw() {
    if( posX < 0 ) posX += 512;
    if( posY < 0 ) posY += 256;

    moveSprite(num, posX, posY);
    setSpriteHFlip(num, hFlip);
    setSpriteVFlip(num, vFlip);
    setSpriteCharacter(num, character);
    setSpritePalette(num, paletteNumber);
}

void Sprite::setPosition(s16 x, s16 y) {
    posX = x;
    posY = y;
}

void Sprite::setPalette(const u8 pn) {
    paletteNumber = pn;
}

void Sprite::setHFlip(bool f) {
    hFlip = f;
}

void Sprite::setVFlip(bool f) {
    vFlip = f;
}

void Sprite::setCharacter(u16 ch) {
    character = ch;
}

void Sprite::flipH() {
    hFlip = !hFlip;
}

void Sprite::flipV() {
    vFlip = !vFlip;
}

s16 Sprite::getPosX() const {
    return posX;
}

s16 Sprite::getPosY() const {
    return posY;
}

u16 Sprite::getSizeBySizeCode(SPRITE_SIZECODE sizeCode) const {
    u16 size = OBJ_SIZE(0);
    switch(sizeCode) {
    case Sprite_8x8:
        size = OBJ_SIZE(0);
        break;
    case Sprite_16x16:
        size = OBJ_SIZE(1);
        break;
    case Sprite_32x32:
        size =  OBJ_SIZE(2);
        break;
    case Sprite_64x64:
        size = OBJ_SIZE(3);
        break;
    case Sprite_16x8:
        size = OBJ_SIZE(0);
        break;
    case Sprite_32x8:
        size = OBJ_SIZE(1);
        break;
    case Sprite_32x16:
        size =  OBJ_SIZE(2);
        break;
    case Sprite_64x32:
        size = OBJ_SIZE(3);
        break;
    case Sprite_8x16:
        size = OBJ_SIZE(0);
        break;
    case Sprite_8x32:
        size = OBJ_SIZE(1);
        break;
    case Sprite_16x32:
        size =  OBJ_SIZE(2);
        break;
    case Sprite_32x64:
        size = OBJ_SIZE(3);
        break;
    }
    return size;
}

u16 Sprite::getShapeBySizeCode(SPRITE_SIZECODE sizeCode) const {
    u16 shape = OBJ_SHAPE(0);
    switch(sizeCode) {
    case Sprite_8x8:
        shape = OBJ_SHAPE(0);
        break;
    case Sprite_16x16:
        shape = OBJ_SHAPE(0);
        break;
    case Sprite_32x32:
        shape = OBJ_SHAPE(0);
        break;
    case Sprite_64x64:
        shape = OBJ_SHAPE(0);
        break;
    case Sprite_16x8:
        shape = OBJ_SHAPE(1);
        break;
    case Sprite_32x8:
        shape = OBJ_SHAPE(1);
        break;
    case Sprite_32x16:
        shape = OBJ_SHAPE(1);
        break;
    case Sprite_64x32:
        shape = OBJ_SHAPE(1);
        break;
    case Sprite_8x16:
        shape =  OBJ_SHAPE(2);
        break;
    case Sprite_8x32:
        shape = OBJ_SHAPE(2);
        break;
    case Sprite_16x32:
        shape = OBJ_SHAPE(2);
        break;
    case Sprite_32x64:
        shape = OBJ_SHAPE(2);
        break;
    }
    return shape;
}

}
