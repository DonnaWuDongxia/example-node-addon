# Node.js Addon BKM #

A Rookie's Guide to First-time Node.js Addon Developer


## How to start a node C++ addon project ##

TODO: add a minimal sample project

## What exactly is an Isolate ##

Quoted from [a StackOverflow post](http://stackoverflow.com/questions/19383724/what-exactly-is-the-difference-between-v8isolate-and-v8context): "An isolate is an independent copy of the V8 runtime, including a heap manager, a garbage collector, etc. Only one thread may access a given isolate at a time, but different threads may access different isolates simultaneously."

So the point is: don't do v8 calls in a worker thread. If you have to do this, use `libuv` features (`uv_async_send()`) in your worker thread to notify the registered notifier function (`void foo(uv_async_t *handle)`) in main thread.

![The bible](http://i.imgur.com/SZPjHwz.jpg)

## What is the difference between node module and node addon ##

 - Node addon: Written in C++, functionalities are carried out by C++ in V8/Nan syntax
 - Node module: written in pure JavaScript, e.g. [This black magic](https://www.npmjs.com/package/left-pad "What a piece of work!")



## How to map JavaScript method to C++ function ##

Assumption: you've already had MyClass derived from Nan::ObjectWrap

	void MyClass::Init(v8::Local<v8::Object> exports) {
	  // ...

	  Nan::SetPrototypeMethod(tpl, "fooMethodInJS", FooMethodInCpp);
	}

The `FooMethodInCpp` should be defined as following:

	void MyClass::FooMethodInCpp(const Nan::FunctionCallbackInfo<v8::Value>& info)
	{
      info.GetReturnValue().Set(Nan::New("Hello as the return value").ToLocalChecked());
	}


## How to convert from JavaScript object to C++, and vice versa ##

Assumption: you've already had MyClass derived from Nan::ObjectWrap

	// Get C++ object back
	MyClass* obj = ObjectWrap::Unwrap<MyClass>(info.Holder());
	// Now do stuffs using obj->xxx

	// This is how to put C++ object in JavaScript object when calling MyClass::New()
	MyClass* obj = new MyClass();
	obj->Wrap(info.This());

## How to get a member of a JavaScript object in C++ ##

Steps:

 - Convert `v8::Value` to `v8::Object`
 - Use `v8::Object::Get()`, provide a name of the member
 - You've got the member (don't forget to do `IsUndefined()` check)

Example:

	void MyClass::PrintMember(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	  if (info[0]->IsObject()) {
	    auto property = Nan::New("memberName").ToLocalChecked();
	    v8::Handle<v8::Value> member = info[0]->ToObject()->Get(property);
	    int valueOfMember = member->Int32Value();
	    printf("In C++, member found with value: %d\n", valueOfMember);
	  }
	}

## How to call a JavaScript function in C++ ##

	func->Call()


## How to wrap a C++ pointer into a JavaScript Value ##

Using `node_pointer.h` with the following modification

TODO: include the modification here...

	TypeName* pointer;
	const int argc = 1UL;
	v8::Local<v8::value> argv[argc] = { WrapPointer(isolate, pointer) };

To obtain the pointer in your C++ side (the pointer is supposed to be holding in JavaScript side code and then passed into C++ side)

	void YourClass::MethodExposedToJavaScript(const Nan::FunctionCallbackInfo<v8::Value>& info)
	{
		// TODO: check whether the following expr is true
		// 		 info[0]->isUndefined()
		TypeName* pointer = UnwrapPointer<TypeName*>(info[0]);
		// Now we can use 'pointer'
	}

## How to return a JavaScript object in C++ ##

Before you can do this, you may need to have `class YourClass` defined

And then do this in your C++ code:

	v8::Local<v8::Function> cons = Nan::New<v8::Function>(YouClass::constructor);
	return const->NewInstance(argc, argv);

Note: `YourClass::constructor` is something like the following (you should have this member declared in your own ObjectWrap-derived class

	class YourClass : public Nan::ObjectWrap {
	// ...
	static Nan::Persistent<v8::Function> constructor;
	// ...
	};

## How to fetch arguments in C++ ##


	void Foo(const Nan::FunctionCallbackInfo<v8::Value>& info) {
 		double value = info[0]->IsUndefined() ? 0 : info[0]->NumberValue();
		// ...
    }

The above code comprises two parts:

 - Check if there is such a value
 - Convert the value to a specified type

Furthermore, you can do more checking by using: `IsNull(), IsTrue(), IsFalse(), IsString(), IsFunction(), ...` [Full list](https://v8docs.nodesource.com/node-4.2/dc/d0a/classv8_1_1_value.html) can be found here.

You can also use `v8::Value::As<typename>()` function to do the casting.

	// After info[0]->IsFunction() check
	auto func = info[0].As<v8::Function>();

Or use `Nan::To<typename>()`, e.g.

	auto floatingNumber = Nan::To<double>(info[0]).FromJust();


## How to convert JavaScript string to C++ string ##

	std::string var;
	v8::String::Utf8Value val(info[0]->ToString());
	var = *val;

## How to convert C++ value to JavaScript value ##

Example#1: when JavaScript expects a string or a number

	info.GetReturnValue().Set(Nan::New("String from C++").ToLocalChecked());
	info.GetReturnValue().Set(Nan::New(3.14159265358979323846264338327950288).ToLocalChecked());

Example#2: wrap a C++ function to JavaScript

    v8::Local<v8::FunctionTemplate> foo = Nan::New<v8::FunctionTemplate>(FooFunc);

`FooFunc` should be declared as:

	void FooFunc (const Nan::FunctionCallbackInfo<v8::Value>& info);


Check [<nan_new.h>](https://github.com/nodejs/nan/blob/master/nan_new.h) for a full list of functions


## How to handle JavaScript array in C++ ##

 - Use `IsArray()` to determine whether we can safely convert a v8::Value to v8::Array
 - Use `v8::Local<v8::Array>::Cast(value)` to convert into v8::Array
 - Use `array->Length()` to get the number of elements inside that array
 - Use `array->Get(0)` to get an element at a index (0 in this example)
 - Use `array->Set(0, value)` to set an element at a index
 - use `v8::Array::New(isolate, capacity)` to create an array in C++. (Note if failed to create the array, `array.IsEmpty()` will be true)
 - 

## How to handle JavaScript typed array in C++ ##

## How to create a Promise in C++ ##

Promise is defined in ECMA-262 6th edition, and is already available in all browsers (including Microsoft Edge).

There is Promise support in V8 and also in NAN. Check out this simple example of the basis usage.

Warning: don't do Resolve() or Reject() in a thread.

	void HelloWorld(const v8::FunctionCallbackInfo<v8::Value>& args) {
    	v8::Isolate* isolate = args.GetIsolate();
		v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(isolate);
		auto promise = resolver->GetPromise();
		// JavaScript will receive this promise and will be able to use thennables
		args.GetReturnValue().Set(promise);

		// Choose one of the following (you can do this later but you'll need Nan::Persistent
		resolver->Resolve(v8::String::NewFromUtf8(isolate, "Hello from librealsense"));
	    resolver->Reject(v8::String::NewFromUtf8(isolate, "Exception from librealsense"));
	}

## Threading in Node.js addon ##




## Useful Links ##
 - [V8 Embedder's Guide](https://developers.google.com/v8/embed)
 - 