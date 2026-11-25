import { app, nativeImage } from 'electron';

const size = { width: 4, height: 4 };
const image = await nativeImage.createFromBufferAsync(Buffer.alloc(size.width * size.height * 4, 0x7f), size);
const encoded = [await image.toPNGAsync(), await image.toJPEGAsync(90), await image.toBitmapAsync()];

process.exit(!app.isReady() && encoded.every((buffer) => buffer.length > 0) ? 0 : 1);
