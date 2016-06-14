var addon = require('./build/Release/exampleAddon');

// The simplest sample function
addon.helloWorld();

console.log(addon.returnValueToJS());

console.log(addon.returnObjectToJS());

addon.getArgFromJS("Double quote is my favorite, what''''''re you gonna do? Bite me?");

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

addon.returnPromiseToJS()
.then(state=>{
	console.log("Promise Resolved:" + state);

})
.catch(error => {
	console.log("Promise Rejected:" + error);
});


var start = Date.now();
var timer = setInterval(function () {
    console.log("Tic-Tock");
}, 1000);
addon.waitInCpp(5000)
.then((value) => {
    console.log(`After ${Date.now() - start}ms: the promise is resolved with value: ${value}`);
    clearInterval(timer);
});

var obj = new addon.MyObject();
console.log(obj + ": " + obj.foo());

