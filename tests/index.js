const sharp = require('sharp')
const supyo = require('../')
const test = require('node:test')
const assert = require('node:assert')

test('detect test', async function () {
  // Load and process test image with sharp
  const { data, info } = await sharp('./tests/lena.jpg')
    .grayscale() // Convert to grayscale
    .raw() // Get raw pixel data
    .toBuffer({ resolveWithObject: true }) // Get buffer with metadata

  console.log('test image loaded', {
    data: data,
    width: info.width,
    height: info.height
      })

      // run image detector
      const detected = supyo.detect(data, info.width, info.height, {
        minSize: 50,
        qualityThreshold: 0,
        verbose: true
      })

      // face should be detected
      assert.ok(detected, 'face detected')
})
