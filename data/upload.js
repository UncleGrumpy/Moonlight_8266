/*
 * Copyright 2020-2024 Winford (Uncle Grumpy) <winford@object.stream>
 * SPDX-License-Identifier: MIT
 */


window.addEventListener('load', startup, false)

function startup () {
  document.getElementById('upButton').disabled = true
  document.getElementById('upButton').className = 'disabled'
  const myFile = document.getElementById('uploads')
  // console.log('Setting up event listener on ' + myFile)
  myFile.addEventListener('change', updateAll, false)
  myFile.select()
}

function updateAll (event) { // this will run on input.
  const file = document.getElementById('uploads')
  // console.log('target is: ' + file)
  let haveFile = file.value
  // console.log('got file: ' + haveFile)
  if (haveFile) {
    // console.log('target is: ' + file)
    // console.log('got file: ' + haveFile)
    console.log('File selection made. Upload enabled.')
    // Enable the submit button.
    document.getElementById('upButton').disabled = false
    document.getElementById('upButton').className = 'enabled'
  } else {
    console.log('No file selection made.')
    console.log('target is: ' + file)
    console.log('got file: ' + haveFile)
    // Disable the submit button.
    document.getElementById('upButton').disabled = true
    document.getElementById('upButton').className = 'disabled'
  }
}
