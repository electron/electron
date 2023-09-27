/*
https://github.com/antimatter15/whammy
The MIT License (MIT)

Copyright (c) 2015 Kevin Kwok

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

function atob (str) {
  return Buffer.from(str, 'base64').toString('binary');
}

// in this case, frames has a very specific meaning, which will be
// detailed once i finish writing the code

function ToWebM (frames) {
  const info = checkFrames(frames);

  // max duration by cluster in milliseconds
  const CLUSTER_MAX_DURATION = 30000;

  const EBML = [
    {
      id: 0x1a45dfa3, // EBML
      data: [
        {
          data: 1,
          id: 0x4286 // EBMLVersion
        },
        {
          data: 1,
          id: 0x42f7 // EBMLReadVersion
        },
        {
          data: 4,
          id: 0x42f2 // EBMLMaxIDLength
        },
        {
          data: 8,
          id: 0x42f3 // EBMLMaxSizeLength
        },
        {
          data: 'webm',
          id: 0x4282 // DocType
        },
        {
          data: 2,
          id: 0x4287 // DocTypeVersion
        },
        {
          data: 2,
          id: 0x4285 // DocTypeReadVersion
        }
      ]
    },
    {
      id: 0x18538067, // Segment
      data: [
        {
          id: 0x1549a966, // Info
          data: [
            {
              data: 1e6, // do things in millisecs (num of nanosecs for duration scale)
              id: 0x2ad7b1 // TimecodeScale
            },
            {
              data: 'whammy',
              id: 0x4d80 // MuxingApp
            },
            {
              data: 'whammy',
              id: 0x5741 // WritingApp
            },
            {
              data: doubleToString(info.duration),
              id: 0x4489 // Duration
            }
          ]
        },
        {
          id: 0x1654ae6b, // Tracks
          data: [
            {
              id: 0xae, // TrackEntry
              data: [
                {
                  data: 1,
                  id: 0xd7 // TrackNumber
                },
                {
                  data: 1,
                  id: 0x73c5 // TrackUID
                },
                {
                  data: 0,
                  id: 0x9c // FlagLacing
                },
                {
                  data: 'und',
                  id: 0x22b59c // Language
                },
                {
                  data: 'V_VP8',
                  id: 0x86 // CodecID
                },
                {
                  data: 'VP8',
                  id: 0x258688 // CodecName
                },
                {
                  data: 1,
                  id: 0x83 // TrackType
                },
                {
                  id: 0xe0, // Video
                  data: [
                    {
                      data: info.width,
                      id: 0xb0 // PixelWidth
                    },
                    {
                      data: info.height,
                      id: 0xba // PixelHeight
                    }
                  ]
                }
              ]
            }
          ]
        },
        {
          id: 0x1c53bb6b, // Cues
          data: [
            // cue insertion point
          ]
        }

        // cluster insertion point
      ]
    }
  ];

  const segment = EBML[1];
  const cues = segment.data[2];

  // Generate clusters (max duration)
  let frameNumber = 0;
  let clusterTimecode = 0;
  while (frameNumber < frames.length) {
    const cuePoint = {
      id: 0xbb, // CuePoint
      data: [
        {
          data: Math.round(clusterTimecode),
          id: 0xb3 // CueTime
        },
        {
          id: 0xb7, // CueTrackPositions
          data: [
            {
              data: 1,
              id: 0xf7 // CueTrack
            },
            {
              data: 0, // to be filled in when we know it
              size: 8,
              id: 0xf1 // CueClusterPosition
            }
          ]
        }
      ]
    };

    cues.data.push(cuePoint);

    const clusterFrames = [];
    let clusterDuration = 0;
    do {
      clusterFrames.push(frames[frameNumber]);
      clusterDuration += frames[frameNumber].duration;
      frameNumber++;
    } while (frameNumber < frames.length && clusterDuration < CLUSTER_MAX_DURATION);

    let clusterCounter = 0;
    const cluster = {
      id: 0x1f43b675, // Cluster
      data: [
        {
          data: Math.round(clusterTimecode),
          id: 0xe7 // Timecode
        }
      ].concat(clusterFrames.map(function (webp) {
        const block = makeSimpleBlock({
          discardable: 0,
          frame: webp.data.slice(4),
          invisible: 0,
          keyframe: 1,
          lacing: 0,
          trackNum: 1,
          timecode: Math.round(clusterCounter)
        });
        clusterCounter += webp.duration;
        return {
          data: block,
          id: 0xa3
        };
      }))
    };

    // Add cluster to segment
    segment.data.push(cluster);
    clusterTimecode += clusterDuration;
  }

  // First pass to compute cluster positions
  let position = 0;
  for (let i = 0; i < segment.data.length; i++) {
    if (i >= 3) {
      cues.data[i - 3].data[1].data[1].data = position;
    }
    const data = generateEBML([segment.data[i]]);
    position += data.size || data.byteLength || data.length;
    if (i !== 2) { // not cues
      // Save results to avoid having to encode everything twice
      segment.data[i] = data;
    }
  }

  return generateEBML(EBML);
}

// sums the lengths of all the frames and gets the duration, woo

function checkFrames (frames) {
  const width = frames[0].width;
  const height = frames[0].height;
  let duration = frames[0].duration;
  for (let i = 1; i < frames.length; i++) {
    if (frames[i].width !== width) throw new Error('Frame ' + (i + 1) + ' has a different width');
    if (frames[i].height !== height) throw new Error('Frame ' + (i + 1) + ' has a different height');
    if (frames[i].duration < 0 || frames[i].duration > 0x7fff) throw new Error('Frame ' + (i + 1) + ' has a weird duration (must be between 0 and 32767)');
    duration += frames[i].duration;
  }
  return {
    duration: duration,
    width: width,
    height: height
  };
}

function numToBuffer (num) {
  const parts = [];
  while (num > 0) {
    parts.push(num & 0xff);
    num = num >> 8;
  }
  return new Uint8Array(parts.reverse());
}

function numToFixedBuffer (num, size) {
  const parts = new Uint8Array(size);
  for (let i = size - 1; i >= 0; i--) {
    parts[i] = num & 0xff;
    num = num >> 8;
  }
  return parts;
}

function strToBuffer (str) {
  // return new Blob([str]);

  const arr = new Uint8Array(str.length);
  for (let i = 0; i < str.length; i++) {
    arr[i] = str.charCodeAt(i);
  }
  return arr;
  // this is slower
  // return new Uint8Array(str.split('').map(function(e){
  //  return e.charCodeAt(0)
  // }))
}

// sorry this is ugly, and sort of hard to understand exactly why this was done
// at all really, but the reason is that there's some code below that i dont really
// feel like understanding, and this is easier than using my brain.

function bitsToBuffer (bits) {
  const data = [];
  const pad = (bits.length % 8) ? (new Array(1 + 8 - (bits.length % 8))).join('0') : '';
  bits = pad + bits;
  for (let i = 0; i < bits.length; i += 8) {
    data.push(parseInt(bits.substr(i, 8), 2));
  }
  return new Uint8Array(data);
}

function generateEBML (json) {
  const ebml = [];
  for (const item of json) {
    if (!('id' in item)) {
      // already encoded blob or byteArray
      ebml.push(item);
      continue;
    }

    let data = item.data;
    if (typeof data === 'object') data = generateEBML(data);
    if (typeof data === 'number') data = ('size' in item) ? numToFixedBuffer(data, item.size) : bitsToBuffer(data.toString(2));
    if (typeof data === 'string') data = strToBuffer(data);

    const len = data.size || data.byteLength || data.length;
    const zeroes = Math.ceil(Math.ceil(Math.log(len) / Math.log(2)) / 8);
    const sizeStr = len.toString(2);
    const padded = (new Array((zeroes * 7 + 7 + 1) - sizeStr.length)).join('0') + sizeStr;
    const size = (new Array(zeroes)).join('0') + '1' + padded;

    // i actually dont quite understand what went on up there, so I'm not really
    // going to fix this, i'm probably just going to write some hacky thing which
    // converts that string into a buffer-esque thing

    ebml.push(numToBuffer(item.id));
    ebml.push(bitsToBuffer(size));
    ebml.push(data);
  }

  // convert ebml to an array
  const buffer = toFlatArray(ebml);
  return new Uint8Array(buffer);
}

function toFlatArray (arr, outBuffer) {
  if (outBuffer == null) {
    outBuffer = [];
  }
  for (const item of arr) {
    if (typeof item === 'object') {
      // an array
      toFlatArray(item, outBuffer);
    } else {
      // a simple element
      outBuffer.push(item);
    }
  }
  return outBuffer;
}

function makeSimpleBlock (data) {
  let flags = 0;
  if (data.keyframe) flags |= 128;
  if (data.invisible) flags |= 8;
  if (data.lacing) flags |= (data.lacing << 1);
  if (data.discardable) flags |= 1;
  if (data.trackNum > 127) {
    throw new Error('TrackNumber > 127 not supported');
  }
  const out = [data.trackNum | 0x80, data.timecode >> 8, data.timecode & 0xff, flags].map(function (e) {
    return String.fromCharCode(e);
  }).join('') + data.frame;

  return out;
}

// here's something else taken verbatim from weppy, awesome rite?

function parseWebP (riff) {
  const VP8 = riff.RIFF[0].WEBP[0];

  const frameStart = VP8.indexOf('\x9d\x01\x2a'); // A VP8 keyframe starts with the 0x9d012a header
  const c = [];
  for (let i = 0; i < 4; i++) c[i] = VP8.charCodeAt(frameStart + 3 + i);

  // the code below is literally copied verbatim from the bitstream spec
  let tmp = (c[1] << 8) | c[0];
  const width = tmp & 0x3FFF;
  const horizontalScale = tmp >> 14;
  tmp = (c[3] << 8) | c[2];
  const height = tmp & 0x3FFF;
  const verticalScale = tmp >> 14;
  return {
    width,
    height,
    horizontalScale,
    verticalScale,
    data: VP8,
    riff: riff
  };
}

// i think i'm going off on a riff by pretending this is some known
// idiom which i'm making a casual and brilliant pun about, but since
// i can't find anything on google which conforms to this idiomatic
// usage, I'm assuming this is just a consequence of some psychotic
// break which makes me make up puns. well, enough riff-raff (aha a
// rescue of sorts), this function was ripped wholesale from weppy

function parseRIFF (string) {
  let offset = 0;
  const chunks = {};

  while (offset < string.length) {
    const id = string.substr(offset, 4);
    chunks[id] = chunks[id] || [];
    if (id === 'RIFF' || id === 'LIST') {
      const len = parseInt(string.substr(offset + 4, 4).split('').map(function (i) {
        const unpadded = i.charCodeAt(0).toString(2);
        return (new Array(8 - unpadded.length + 1)).join('0') + unpadded;
      }).join(''), 2);
      const data = string.substr(offset + 4 + 4, len);
      offset += 4 + 4 + len;
      chunks[id].push(parseRIFF(data));
    } else if (id === 'WEBP') {
      // Use (offset + 8) to skip past "VP8 "/"VP8L"/"VP8X" field after "WEBP"
      chunks[id].push(string.substr(offset + 8));
      offset = string.length;
    } else {
      // Unknown chunk type; push entire payload
      chunks[id].push(string.substr(offset + 4));
      offset = string.length;
    }
  }
  return chunks;
}

// here's a little utility function that acts as a utility for other functions
// basically, the only purpose is for encoding "Duration", which is encoded as
// a double (considerably more difficult to encode than an integer)
function doubleToString (num) {
  return Array.prototype.slice.call(
    new Uint8Array(
      (
        new Float64Array([num]) // create a float64 array
      ).buffer) // extract the array buffer
    , 0) // convert the Uint8Array into a regular array
    .map(function (e) { // since it's a regular array, we can now use map
      return String.fromCharCode(e); // encode all the bytes individually
    })
    .reverse() // correct the byte endianness (assume it's little endian for now)
    .join(''); // join the bytes in holy matrimony as a string
}

function WhammyVideo (speed, quality = 0.8) { // a more abstract-ish API
  this.frames = [];
  this.duration = 1000 / speed;
  this.quality = quality;
}

/**
 *
 * @param {string} frame
 * @param {number} [duration]
 */
WhammyVideo.prototype.add = function (frame, duration) {
  if (typeof duration !== 'undefined' && this.duration) throw new Error("you can't pass a duration if the fps is set");
  if (typeof duration === 'undefined' && !this.duration) throw new Error("if you don't have the fps set, you need to have durations here.");
  if (frame.canvas) { // CanvasRenderingContext2D
    frame = frame.canvas;
  }
  if (frame.toDataURL) {
    // frame = frame.toDataURL('image/webp', this.quality);
    // quickly store image data so we don't block cpu. encode in compile method.
    frame = frame.getContext('2d').getImageData(0, 0, frame.width, frame.height);
  } else if (typeof frame !== 'string') {
    throw new TypeError('frame must be a a HTMLCanvasElement, a CanvasRenderingContext2D or a DataURI formatted string');
  }
  if (typeof frame === 'string' && !(/^data:image\/webp;base64,/ig).test(frame)) {
    throw new Error('Input must be formatted properly as a base64 encoded DataURI of type image/webp');
  }
  this.frames.push({
    image: frame,
    duration: duration || this.duration
  });
};

WhammyVideo.prototype.compile = function (callback) {
  const webm = new ToWebM(this.frames.map(function (frame) {
    const webp = parseWebP(parseRIFF(atob(frame.image.slice(23))));
    webp.duration = frame.duration;
    return webp;
  }));
  callback(webm);
};

export const WebmGenerator = WhammyVideo;
