var addon = require('./build/Release/exampleAddon');

// The simplest sample function
addon.helloWorld();

console.log(addon.returnValueToJS());

console.log(addon.returnObjectToJS());

addon.getArgFromJS("Double quote is my favorite, what're you gonna do? Bite me?");

addon.consumeArrayFromJS(['Apple', 'Tangerine', 'Durian', 'Venus fly trap']);

addon.extractMemberInObject({
	piValue: 3.14159265358979323846264338327950288,
	saySomething: function () {
		console.log("Something!");
	}
});

addon.invokeJSFunc(function(str, int){
	console.log("I've got arguments from C++, str='" + str + "', int=" + int);
})

