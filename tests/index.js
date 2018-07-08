'use strict'

var fs = require('fs')
var test = require('tape')

// checkout requirements if failing to install
// https://github.com/sorccu/node-jpeg-turbo#requirements
var jpg = require('jpeg-turbo')

var supyo = require('../')

test('detect test', function (t) {
  t.plan(1)

  // load a test image
  var jpegData = fs.readFileSync('./tests/lena.jpg')
  var imageData = jpg.decompressSync(jpegData, {
    format: jpg.FORMAT_GRAY
  })
  console.log('test image loaded', imageData)

  // run image detector
  var detected = supyo.detect(imageData.data, imageData.width, imageData.height, {
    minSize: 50,
    qualityThreshold: 0,
    verbose: true
  })

  // face should be detected
  t.true(detected, 'face detected')
})
