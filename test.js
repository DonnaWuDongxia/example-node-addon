var addon = require('./build/Release/exampleAddon');

// The simplest sample function
addon.helloWorld();

console.log(addon.returnValueToJS());

console.log(addon.returnObjectToJS());

addon.getArgFromJS("Double quote is my favorite, what're you gonna do? Bite me?");

