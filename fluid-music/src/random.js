/**
 * Randomly get an element from an object or array.
 * @param {Object|Array} input 
 */
function choice(input) {
  if (!input.length && !(Object.keys(input).length))
    return undefined;
  if (Array.isArray(input))
    return input[Math.floor(Math.random() * input.length)];
  else
    return input[randomChoice(Object.keys(input))];
}

module.exports = {
  choice,
}