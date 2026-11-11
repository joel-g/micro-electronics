#include "IndexedFramebuffer.h"

#include <Arduino.h>
#include <cstring>

namespace {
void recordMax(uint32_t &maximum, uint32_t value) {
    if (value > maximum) maximum = value;
}
}

void IndexedFramebuffer::present() {
    nextTile = 0;
    frameTiles = 0;
    sending = true;
    frameStarted = micros();
}

void IndexedFramebuffer::invalidate() {
    cancel();
    force = true;
}

void IndexedFramebuffer::flushBatch(unsigned budget, void *target, TileWriter writeTile) {
    if (!sending) return;

    const uint32_t started = micros();
    unsigned transmitted = 0;
    const auto *pixels = buffer.getBuffer();
    while (nextTile < TileCount && transmitted < budget) {
        const int x = (nextTile % TilesAcross) * TileWidth;
        const int y = (nextTile / TilesAcross) * TileHeight;
        ++nextTile;

        bool changed = force;
        for (int row = 0; row < TileHeight && !changed; ++row) {
            const int index = (y + row) * Width + x;
            changed = std::memcmp(pixels + index, previous + index, TileWidth) != 0;
        }
        if (!changed) continue;

        for (int row = 0; row < TileHeight; ++row) {
            const int index = (y + row) * Width + x;
            for (int column = 0; column < TileWidth; ++column) {
                tilePixels[row * TileWidth + column] = rgb565Palette[pixels[index + column]];
            }
        }
        writeTile(target, x, y, TileWidth, TileHeight, tilePixels);
        for (int row = 0; row < TileHeight; ++row) {
            const int index = (y + row) * Width + x;
            std::memcpy(previous + index, pixels + index, TileWidth);
        }
        ++tiles;
        ++frameTiles;
        ++transmitted;
    }

    if (nextTile == TileCount) {
        if (frameTiles) ++frames;
        ++completed;
        recordMax(maxFrameUs, micros() - frameStarted);
        sending = false;
        force = false;
    }
    recordMax(maxTransferUs, micros() - started);
}
