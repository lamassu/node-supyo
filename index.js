'use strict'

var supyo = require('./build/Release/supyo')

/**
 * @param {Buffer} image
 * @param {number} width
 * @param {number} height
 * @param {object} [opts]
 * @param {number} [opts.minSize=100] min face size (in pixel)
 * @param {number} [opts.qualityThreshold=5.0] threshold to discard low quality results
 * @param {boolean} [opts.verbose=false] hide/show debug information
 * @returns {boolean} face has been detected or not
 */
exports.detect = function detect (image, width, height, opts) {
  opts = opts || {}
  var minSize = opts.minSize || 100
  var qualityThreshold = opts.qualityThreshold || 5.0
  var verbose = opts.verbose || false
  return supyo.detect(image, width, height, minSize, qualityThreshold, verbose)
}
