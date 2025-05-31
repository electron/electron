# ColorSpace Object

* `primaries` string - The color primaries of the color space. Can be one of the following values:
  * `bt709` - BT709 primaries (also used for sRGB)
  * `bt470m` - BT470M primaries
  * `bt470bg` - BT470BG primaries
  * `smpte170m` - SMPTE170M primaries
  * `smpte240m` - SMPTE240M primaries
  * `film` - Film primaries
  * `bt2020` - BT2020 primaries
  * `smptest428-1` - SMPTEST428-1 primaries
  * `smptest431-2` - SMPTEST431-2 primaries
  * `p3` - P3 primaries
  * `xyz-d50` - XYZ D50 primaries
  * `adobe-rgb` - Adobe RGB primaries
  * `apple-generic-rgb` - Apple Generic RGB primaries
  * `wide-gamut-color-spin` - Wide Gamut Color Spin primaries
  * `ebu-3213-e` - EBU 3213-E primaries
  * `custom` - Custom primaries
  * `invalid` - Invalid primaries

* `transfer` string - The transfer function of the color space. Can be one of the following values:
  * `bt709` - BT709 transfer function
  * `bt709-apple` - BT709 Apple transfer function
  * `gamma18` - Gamma 1.8 transfer function
  * `gamma22` - Gamma 2.2 transfer function
  * `gamma24` - Gamma 2.4 transfer function
  * `gamma28` - Gamma 2.8 transfer function
  * `smpte170m` - SMPTE170M transfer function
  * `smpte240m` - SMPTE240M transfer function
  * `linear` - Linear transfer function
  * `log` - Log transfer function
  * `log-sqrt` - Log Square Root transfer function
  * `iec61966-2-4` - IEC61966-2-4 transfer function
  * `bt1361-ecg` - BT1361 ECG transfer function
  * `srgb` - sRGB transfer function
  * `bt2020-10` - BT2020-10 transfer function
  * `bt2020-12` - BT2020-12 transfer function
  * `pq` - PQ (Perceptual Quantizer) transfer function
  * `smptest428-1` - SMPTEST428-1 transfer function
  * `hlg` - HLG (Hybrid Log-Gamma) transfer function
  * `srgb-hdr` - sRGB HDR transfer function
  * `linear-hdr` - Linear HDR transfer function
  * `custom` - Custom transfer function
  * `custom-hdr` - Custom HDR transfer function
  * `scrgb-linear-80-nits` - scRGB Linear 80 nits transfer function
  * `invalid` - Invalid transfer function

* `matrix` string - The color matrix of the color space. Can be one of the following values:
  * `rgb` - RGB matrix
  * `bt709` - BT709 matrix
  * `fcc` - FCC matrix
  * `bt470bg` - BT470BG matrix
  * `smpte170m` - SMPTE170M matrix
  * `smpte240m` - SMPTE240M matrix
  * `ycocg` - YCoCg matrix
  * `bt2020-ncl` - BT2020 NCL matrix
  * `ydzdx` - YDzDx matrix
  * `gbr` - GBR matrix
  * `invalid` - Invalid matrix

* `range` string - The color range of the color space. Can be one of the following values:
  * `limited` - Limited color range (RGB values ranging from 16 to 235)
  * `full` - Full color range (RGB values from 0 to 255)
  * `derived` - Range defined by the transfer function and matrix
  * `invalid` - Invalid range

## Common `ColorSpace` definitions

### Standard Color Spaces

**sRGB**:

  ```js
  const cs = {
    primaries: 'bt709',
    transfer: 'srgb',
    matrix: 'rgb',
    range: 'full'
  }
  ```

**Display P3**:

  ```js
  const cs = {
    primaries: 'p3',
    transfer: 'srgb',
    matrix: 'rgb',
    range: 'full'
  }
  ```

**XYZ D50**:

  ```js
  const cs = {
    primaries: 'xyz-d50',
    transfer: 'linear',
    matrix: 'rgb',
    range: 'full'
  }
  ```

### HDR Color Spaces

**Extended sRGB** (extends sRGB to all real values):

  ```js
  const cs = {
    primaries: 'bt709',
    transfer: 'srgb-hdr',
    matrix: 'rgb',
    range: 'full'
  }
  ```

**scRGB Linear** (linear transfer function for all real values):

  ```js
  const cs = {
    primaries: 'bt709',
    transfer: 'linear-hdr',
    matrix: 'rgb',
    range: 'full'
  }
  ```

**scRGB Linear 80 Nits** (with an SDR white level of 80 nits):

  ```js
  const cs = {
    primaries: 'bt709',
    transfer: 'scrgb-linear-80-nits',
    matrix: 'rgb',
    range: 'full'
  }
  ```

**HDR10** (BT.2020 primaries with PQ transfer function):

  ```js
  const cs = {
    primaries: 'bt2020',
    transfer: 'pq',
    matrix: 'rgb',
    range: 'full'
  }
  ```

**HLG** (BT.2020 primaries with HLG transfer function):

  ```js
  const cs = {
    primaries: 'bt2020',
    transfer: 'hlg',
    matrix: 'rgb',
    range: 'full'
  }
  ```

### Video Color Spaces

**Rec. 601** (SDTV):

  ```js
  const cs = {
    primaries: 'smpte170m',
    transfer: 'smpte170m',
    matrix: 'smpte170m',
    range: 'limited'
  }
  ```

**Rec. 709** (HDTV):

  ```js
  const cs = {
    primaries: 'bt709',
    transfer: 'bt709',
    matrix: 'bt709',
    range: 'limited'
  }
  ```

**JPEG** (typical color space for JPEG images):

  ```js
  const cs = {
    primaries: 'bt709',
    transfer: 'srgb',
    matrix: 'smpte170m',
    range: 'full'
  }
  ```
