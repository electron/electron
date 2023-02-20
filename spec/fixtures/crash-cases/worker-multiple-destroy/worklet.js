class CheckerboardPainter {
  paint (ctx, geom, properties) {
    const colors = ['red', 'green', 'blue'];
    const size = 32;
    for (let y = 0; y < (geom.height / size); y++) {
      for (let x = 0; x < (geom.width / size); x++) {
        const color = colors[(x + y) % colors.length];
        ctx.beginPath();
        ctx.fillStyle = color;
        ctx.rect(x * size, y * size, size, size);
        ctx.fill();
      }
    }
  }
}

// eslint-disable-next-line no-undef
registerPaint('checkerboard', CheckerboardPainter);
