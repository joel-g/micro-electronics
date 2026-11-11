#pragma once

#include <Adafruit_GFX.h>
#include <stdint.h>

class IndexedFramebuffer {
public:
    // The palette must outlive the framebuffer and cover every drawn index.
    explicit IndexedFramebuffer(const uint16_t *palette) : rgb565Palette(palette) {}
    IndexedFramebuffer(const IndexedFramebuffer &) = delete;
    IndexedFramebuffer &operator=(const IndexedFramebuffer &) = delete;

    // Leave the canvas unchanged while busy, or cancel before redrawing.
    GFXcanvas8 &canvas() { return buffer; }
    bool busy() const { return sending; }
    void present();
    void cancel() { sending = false; }
    // Discard the shadow after the panel is reset or cleared.
    void invalidate();

    // The writer must consume the pixels before returning. The shadow is
    // updated after each write so a canceled frame can be replaced safely.
    using TileWriter = void (*)(void *, int, int, int, int, uint16_t *);
    void flushBatch(unsigned budget, void *target, TileWriter writeTile);

    template <typename Driver>
    void flushBatch(unsigned budget, Driver &driver) {
        flushBatch(budget, &driver, [](void *target, int x, int y, int width,
                                      int height, uint16_t *pixels) {
            auto &panel = *static_cast<Driver *>(target);
            panel.startWrite();
            panel.setAddrWindow(x, y, width, height);
            panel.writePixels(pixels, width * height, true);
            panel.endWrite();
        });
    }

    uint32_t frames = 0;
    uint32_t completed = 0;
    uint32_t tiles = 0;
    uint32_t maxTransferUs = 0;
    uint32_t maxFrameUs = 0;

private:
    static constexpr int Width = 240;
    static constexpr int Height = 240;
    static constexpr int TileWidth = 16;
    static constexpr int TileHeight = 8;
    static constexpr int TilesAcross = Width / TileWidth;
    static constexpr int TileCount = TilesAcross * (Height / TileHeight);

    class Canvas : public GFXcanvas8 {
    public:
        Canvas() : GFXcanvas8(Width, Height, false) { buffer = pixels; }

    private:
        alignas(4) uint8_t pixels[Width * Height] = {};
    } buffer;

    uint8_t previous[Width * Height] = {};
    uint16_t tilePixels[TileWidth * TileHeight] = {};
    const uint16_t *const rgb565Palette;
    uint16_t nextTile = 0;
    uint32_t frameTiles = 0;
    uint32_t frameStarted = 0;
    bool sending = false;
    bool force = true;
};
