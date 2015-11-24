#include <gba.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstdarg>
#include "sprites.h"
#include "bg.h"
#include "vba.h"
#include "stage.hpp"
#include "Sprite.hpp"

namespace {
/* フレーム数 */
u16 frame;

const u16 BUFSIZE = 1024;

/**
 * VBA のデバッグコンソールに出力する
 * @param const char* s 出力する文字列
 */
void cprintf(const char* format, ...) {
    va_list ap;
    char buf[BUFSIZE];

    va_start(ap, format);
    vsnprintf(buf, BUFSIZE, format, ap);
    va_end(ap);

    vbalog(buf);
}
}

void drawBGCloud(int x, int y) {
    MAP[31][y    ][x    ] = 4;
    MAP[31][y    ][x + 1] = 5;
    MAP[31][y    ][x + 2] = 6;
    MAP[31][y    ][x + 3] = 7;
    MAP[31][y + 1][x    ] = 4 + 32;
    MAP[31][y + 1][x + 1] = 5 + 32;
    MAP[31][y + 1][x + 2] = 6 + 32;
    MAP[31][y + 1][x + 3] = 7 + 32;
}

void setBackground() {
    drawBGCloud(1, 4);
    drawBGCloud(10, 2);
    drawBGCloud(15, 1);
    drawBGCloud(17, 3);
    drawBGCloud(21, 0);
    drawBGCloud(24, 6);
    drawBGCloud(29, 3);
}

s16 calcActualPosX(u16 mw, s16 vx) {
    if ( vx < ( SCREEN_WIDTH / 2 ) ) return vx;
    if ( ( mw - ( SCREEN_WIDTH / 2 ) ) < vx ) return vx - (mw - ( SCREEN_WIDTH ));
    return ( SCREEN_WIDTH / 2 );
}

s16 calcBgOffset(u16 mw, u16 my, s16 vx, s16 vy, u16 co) {
    if ( vx < ( SCREEN_WIDTH / 2 ) ) return co;
    if ( ( mw - ( SCREEN_WIDTH / 2 ) ) < vx ) return co;
    return vx - ( SCREEN_WIDTH / 2 );
}

/**
 * ステージの初期化
 */
void initStage() {
    u16 xx, yy;
    for ( yy = 0; yy < 20; yy++ ) {
        for ( xx = 0; xx < 60; xx++ ) {
            // 1 MAP の容量は 32 byte
            if ( xx < 32 ) {
                MAP[29][yy][xx] = stage[yy][xx];
            } else {
                MAP[30][yy][xx - 32] = stage[yy][xx];
            }
        }
    }
}

/**
 * ステージの描画
 *
 * @param u16 mw     マップ幅
 * @param u16 offset 画面オフセット
 * @param s8  mov    移動方向(正なら進む、負なら戻る、0 なら移動していない)
 */
void drawStage(u16 mw, u16 offset, s8 mov) {
    if ( mov == 0 ) return;     // 移動していない
    if ( offset % 8 != 0 ) {
//        return;  // マス目の境界上でない
    }
    u16 m;     // オフセットのマス換算
    s16 dstart = 0, dend, sstart;
    if ( 0 < mov ) {
        // 進んだ
        m = (offset - 1) >> 3;
        dstart = (m + 35) % 64;
        sstart = m + 35;
    } else {
        // 戻った
        m = offset >> 3;
        dstart = (m - 25) % 64;
        sstart = m - 25;
    }
    if ( ( m % 15 ) != 0 ) {
        return; // 再描画ポイント上でない
    }
    cprintf("offset = %d, m = %d: sstart = %d, dstart = %d\n", offset, m, sstart, dstart);

    s16 dx, dy, ddx, sx, sy;
    u16 maxStagePos = mw >> 3;

    dend = dstart + 15;
    for ( sy = dy = 0; (dy < 20) && ( sy < 20); dy++, sy++ ) {
        for ( sx = sstart, dx = dstart;
              (dx < dend) && (sx < maxStagePos);
              dx++, sx++ ) {
            if ( sx < 0 ) continue;

            if ( dx < 0 ) {
                ddx = dx + 64;
            } else if ( dx < 64 ) {
                ddx = dx;
            } else {
                ddx = dx - 64;
            }
            if ( ddx < 32 ) {
                MAP[29][dy][ddx] = stage[sy][sx];
            } else {
                MAP[30][dy][ddx - 32] = stage[sy][sx];
            }
        }
    }
}

int main() {
    /* ドロイド君の x, y 座標 */
    s16 dx = 120, dy = 88;
    /* りんごの x, y 座標 */
    const s16 ax = 160, ay = 120;
    /* 窓の x, y 座標 */
    const s16 wx = 40, wy = 40;
    /* タイルのベースアドレス */
    u16* tile = (u16*)TILE_BASE_ADR(0);
    /*
     * ドロイド君の状態。
     * 0 => 待機
     * 1 => ジャンプ準備中
     * 2 => ジャンプ中
     */
    int state = 0;
    /* 歩き状態 (0, 1, 2) */
    int wstate = 0;
    /* ドロイド君の y 方向の速度 */
    float vy = 0;
    /* フレーム数用の変数 */
    u16 f = 0;
    /* 表示するキャラクタ */
    u16 ch = 0;
    /* キー状態取得用変数 */
    u16 kd = 0;
    u16 kdr = 0;
    u16 ku = 0;
    u16 kh = 0;
    /* 汎用変数 */
    u16 i, xx, yy;

    /* フレーム数初期化 */
    frame = 0;

    /* 割り込みの初期化 */
    irqInit();
    /*
     * VBLANK 割り込みを有効化
     * これによって VBlankIntrWait() が使えるようになる
     */
    irqEnable(IRQ_VBLANK);

    /*
     * モードの設定。
     * MODE 0、スプライト有効化、BG0-1 有効化
     */
    SetMode(MODE_0 | OBJ_ENABLE | BG0_ON | BG1_ON);

    /* スプライトのメモリ領域(OAM)にスプライトデータをコピー */
    GbaGraphics::setSpriteData(spritesTiles, spritesPal, spritesTilesLen / 2);

    /* スプライトと BG のパレットのメモリ領域にパレットデータをコピー */
    GbaGraphics::setBGPalette(bgPal, 16);
    GbaGraphics::setBGColor(RGB5(20,20,31));

    /* スプライトの初期化 */
    GbaGraphics::initSprites();

    /* ドロイド君の準備 */
    GbaGraphics::Sprite droid(0, 0, 0, Sprite_16x16);
    droid.setPosition(dx, dy);
    droid.draw();

    /* BG 0 の設定 */
    BGCTRL[0] = BG_MAP_BASE(31) | BG_16_COLOR | BG_SIZE_0 | TILE_BASE(0) | BG_PRIORITY(0);
    /* BG 1 の設定 */
    BGCTRL[1] = BG_MAP_BASE(29) | BG_16_COLOR | BG_SIZE_1 | TILE_BASE(0) | BG_PRIORITY(1);

    /* BG0-1 を初期化 */
    for ( xx = 0; xx < 32; xx++ ) {
        for ( yy = 0; yy < 32; yy++ ) {
            MAP[31][yy][xx] = 0;
            MAP[30][yy][xx] = 0;
        }
    }

    /* 画像をタイルにコピー */
    for ( i = 0; i < bgTilesLen / 2; i++ ) {
        tile[i] = bgTiles[i];
    }

    // 遠景のセット
    setBackground();
    // 背景のセット
    initStage();

    // コンソール出力のテスト
    cprintf("droid(%d, %d), apple(%d, %d), window(%d, %d), %s\n", dx, dy, ax, ay, wx, wy, "console test.");

    /* メインループ */
    u16 vMapWidth = 1728;        // 仮想マップ幅
    u16 vMapHeight = 160;        // 仮想マップ高さ
    s16 vPlayerPosX = 120;       // 仮想プレイヤー x 座標
    s16 vPlayerPosY = 88;       // 仮想プレイヤー y 座標
    u16 bgOffset = 0;
    u16 prevBgOffset = 0;
    u16 baisoku = 0;
    while (1) {
        /* VBLANK 割り込み待ち */
        VBlankIntrWait();
        /* フレーム数カウント */
        frame += 1;
        /* キー状態取得 */
        scanKeys();
        kd = keysDown();
        kdr = keysDownRepeat();
        ku = keysUp();
        kh = keysHeld();

        // BG スクロール
        prevBgOffset = bgOffset;
        bgOffset = calcBgOffset(vMapWidth, vMapHeight, vPlayerPosX, vPlayerPosX, bgOffset);
        REG_BG0HOFS = bgOffset / 3;
        REG_BG1HOFS = bgOffset;
        // ステージ描画
        drawStage(vMapWidth, bgOffset, bgOffset - prevBgOffset);
        switch(state) {
        case 0:
            /* 待機中 */
            if( (kd & KEY_A) ) {
                state = 1;
                f = 0;
                ch = 0;
                break;
            }
            if( (ku & KEY_UP) ) {
                droid.setPalette(1);
                break;
            }
            if( (ku & KEY_DOWN) ) {
                droid.setPalette(0);
                break;
            }
            if( (kh & KEY_LEFT) ) {
                vPlayerPosX--;
                if ( (kh & KEY_B) ) {
                    vPlayerPosX--;
                }
                droid.setHFlip(true);
            }
            if( (kh & KEY_RIGHT) ) {
                vPlayerPosX++;
                if ( (kh & KEY_B) ) {
                    vPlayerPosX++;
                }
                droid.setHFlip(false);
            }
            if( vPlayerPosX < 0 ) {
                vPlayerPosX = 0;
            }
            if ( vMapWidth - 16 < vPlayerPosX ) {
                vPlayerPosX = vMapWidth - 16;
            }

            if( kd & ( KEY_LEFT | KEY_RIGHT ) ) {
                wstate = 0;
                f = 0;
            }
            if( ku & ( KEY_LEFT | KEY_RIGHT ) ) {
                ch = 0;
            }

            if ( kh & ( KEY_LEFT | KEY_RIGHT ) ) {
                /* 歩きモーション */
                if ( (kh & KEY_B) ) {
                    baisoku = 1;
                } else {
                    baisoku = 0;
                }
                if ( 5 / (baisoku + 1) < f++ ) {
                    switch ( wstate ) {
                    case 0:
                        wstate = 1;
                        ch = 2;
                        break;
                    case 1:
                        wstate = 2;
                        ch = 0;
                        break;
                    case 2:
                        wstate = 3;
                        ch = 4;
                        break;
                    default:
                        wstate = 0;
                        ch = 0;
                        break;
                    }
                    f = 0;
                }
            }

            droid.setCharacter(ch);
            dx = calcActualPosX(vMapWidth, vPlayerPosX);
            droid.setPosition(dx, dy);
            break;
        case 1:
        case 3:
            /* ジャンプ準備 */
            droid.setCharacter(6);
            if ( (kh & KEY_B) ) {
                baisoku = 1;
            } else {
                baisoku = 0;
            }
            if( 2 - (baisoku * 2) < f++ ) {
                vy = 4.;
                if ( 1 == state ) state = 2;
                else state = 4;
            }
            break;
        case 2:
        case 4:
            /* ジャンプ中 */
            if( (kd & KEY_A) ) {
                /* 二段ジャンプ */
                if ( state == 2 ) {
                    state = 3;
                    f = 0;
                    break;
                }
            }
            if( kh & KEY_LEFT ) {
                vPlayerPosX--;
                if ( (kh & KEY_B) ) {
                    vPlayerPosX--;
                }
                droid.setHFlip(true);
            }
            if( kh & KEY_RIGHT ) {
                vPlayerPosX++;
                if ( (kh & KEY_B) ) {
                    vPlayerPosX++;
                }
                droid.setHFlip(false);
            }
            if( vPlayerPosX < 0 ) {
                vPlayerPosX = 0;
            }
            if ( vMapWidth - 16 < vPlayerPosX ) {
                vPlayerPosX = vMapWidth - 16;
            }
            if( (0.5 < vy) && ( kh & KEY_A ) ) {
                vy += 0.2;
            }
            vPlayerPosY -= (s16)vy;
            if(vy < 0) {
                droid.setCharacter(10);
            } else {
                droid.setCharacter(8);
            }
            if ( vPlayerPosY < 0 ) {
                vPlayerPosY = 0;
                vy = -0.;
            }

            if ( 88 < vPlayerPosY ) {
                /* 着地 */
                vPlayerPosY = 88;
                state = 0;
            }
            dx = calcActualPosX(vMapWidth, vPlayerPosX);
            dy = vPlayerPosY;
            droid.setPosition(dx, dy);
            vy = vy - 0.3;
            break;
        }
        droid.draw();
    }
}
