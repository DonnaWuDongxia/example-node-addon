#include <node.h>
#include <nan.h>

#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <chrono>
#include <tuple>
#include <random>

// Show nothing else but a function can be invoked by JavaScript
/*

<nan.h>: 1297

#define NAN_METHOD(name)                                                       \
    Nan::NAN_METHOD_RETURN_TYPE name(Nan::NAN_METHOD_ARGS_TYPE info)

*/
NAN_METHOD(HelloWorld) {
  printf("Old fashion way to say hello in C\n");
}

// Returns a simple type value to JavaScript
NAN_METHOD(ReturnValueToJS) {
  // info is the default parameter name defined in <nan.h>:1297
  info.GetReturnValue().Set(Nan::New("Hello from C++").ToLocalChecked());
}

// Returns an object
NAN_METHOD(ReturnObjectToJS) {
  info.GetReturnValue().Set(Nan::New<v8::Object>());
}

//
// Refer to other form of Nan::New<typename> to create other types of values
//

// Get an argument from JavaScript
NAN_METHOD(GetArgFromJS) {
  // Check if we got one argument
  if (info.Length() == 1) {
    printf("We've got %d arguments from JavaScript\n", info.Length());
    if (info[0]->IsString()) {
      v8::String::Utf8Value value(info[0]);
      std::string str(*value);
      printf("String came from JavaScript: %s\n", str.c_str());
    }
  } else {
    printf("Usage: getArgFromJS(1 string argument)\n");
  }
}

// Demonstrates how to deal with JavaScript array passed into C++
NAN_METHOD(ConsumeArrayFromJS) {
  if (info.Length() == 1 && info[0]->IsArray()) {
    v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(info[0]);
    int len = array->Length();
    printf("Array came from JavaScript with length=%d; ", len);

    for ( int i = 0 ; i < len ; ++ i ) {
      // Yep, this `Get()` function is how to extract an element from an array
      auto elem = array->Get(i);
      if (elem->IsString()) {
        v8::String::Utf8Value value(elem);
        std::string str(*value);
        printf("%d=%s, ", i, str.c_str());
      }
    }
    printf("\n");
  } else {
    printf("Usage: consumeArrayFromJS(['str1', 'str2', '...'])\n");
  }
}

// Get an object from JavaScript and use the member of the object in C++
NAN_METHOD(ExtractMemberInObject) {
  if (info.Length() == 1 && info[0]->IsObject()) {
    auto object = info[0]->ToObject();
    // We can use `Get()` with a name as the argument to get members from an object
    auto pi = object->Get(Nan::New("piValue").ToLocalChecked());
    if (pi->IsNumber()) {
      printf("object.piValue=%lf\n", pi->NumberValue());
    }
    //auto func = object->Get(Nan::New("saySomething").ToLocalChecked());
  } else {
    printf("Usage: extractMemberInObject({xxx: <value1>, yyy: <value2>, ...})\n");
  }
}

// Demonstrate how to invoke a JavaScript function inside C++
NAN_METHOD(InvokeJSFunc) {
  if (info.Length() == 1 && info[0]->IsFunction()) {
    auto func = v8::Local<v8::Function>::Cast(info[0]);
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = {
      Nan::New("C++").ToLocalChecked(),
      Nan::New(2.71828182845904523536d) // It's not an integer, so what?
    };
    func->Call(info.This(), argc, argv);
  } else {
    printf("Usage: invokeJSFunc(function(str, int){})\n");
  }
}

// Advanced topics
NAN_METHOD(ReturnPromiseToJS) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(isolate);
  info.GetReturnValue().Set(resolver->GetPromise());

  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(0, 100.0);
  if ( dist(mt) < 50 ) {
    resolver->Resolve(Nan::New("resolved by C++").ToLocalChecked());
  } else {
    resolver->Reject(Nan::New("rejected by C++").ToLocalChecked());
  }
}

NAN_METHOD(WaitInCpp) {
  using ResolverPersistent = Nan::Persistent<v8::Promise::Resolver>;

  auto period = Nan::To<unsigned>(info[0]).FromJust(); // In ms
  auto resolver = v8::Promise::Resolver::New(info.GetIsolate());
  auto persistent = new ResolverPersistent(resolver);

  uv_timer_t* handle = new uv_timer_t;
  handle->data = persistent;
  uv_timer_init(uv_default_loop(), handle);

  // use capture-less lambda for c-callback
  auto timercb = [](uv_timer_t* handle) -> void {
    Nan::HandleScope scope;

    auto persistent = static_cast<ResolverPersistent*>(handle->data);
    
    uv_timer_stop(handle);
    uv_close(reinterpret_cast<uv_handle_t*>(handle),
             [](uv_handle_t* handle) -> void {delete handle;});
  
    auto resolver = Nan::New(*persistent);
    resolver->Resolve(Nan::New("'C++ Resolved String Value'").ToLocalChecked());

    persistent->Reset();
    delete persistent;
  };
  uv_timer_start(handle, timercb, period, 0);

  info.GetReturnValue().Set(resolver->GetPromise());
}

class MyObject : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);

 private:
  explicit MyObject();
  ~MyObject();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Foo(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void ToString(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static NAN_GETTER(XGetter);
  static NAN_SETTER(XSetter);

  static Nan::Persistent<v8::Function> constructor;

 private:
  double value_;
};

Nan::Persistent<v8::Function> MyObject::constructor;

MyObject::MyObject() {
  value_ = 2.3333333;
}

MyObject::~MyObject() {
}

void MyObject::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("MyObject").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype method
  Nan::SetPrototypeMethod(tpl, "foo", Foo);
  Nan::SetPrototypeMethod(tpl, "toString", ToString);
  Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New("x").ToLocalChecked(), XGetter, XSetter);

  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("MyObject").ToLocalChecked(), tpl->GetFunction());
}

void MyObject::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (info.IsConstructCall()) {
    // Invoked as constructor: `new MyObject()`
    MyObject* obj = new MyObject();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    // Invoked as plain function `MyObject()`, turn into construct call.
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { info[0] };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    info.GetReturnValue().Set(cons->NewInstance(argc, argv));
  }
}

NAN_GETTER(MyObject::XGetter) {
  // property, info
  auto myself = ObjectWrap::Unwrap<MyObject>(info.Holder());
  info.GetReturnValue().Set(Nan::New(myself->value_));
}

NAN_SETTER(MyObject::XSetter) {
  // property, value, info
  auto myself = ObjectWrap::Unwrap<MyObject>(info.Holder());
  if (value->IsNumber()) {
    myself->value_ = value->NumberValue();
  }
}

NAN_METHOD(MyObject::Foo) {
  info.GetReturnValue().Set(Nan::New("I am Thor, son of Odin, and as long as there's life in my breast... I'm running out of things to say").ToLocalChecked());
}

NAN_METHOD(MyObject::ToString) {
  info.GetReturnValue().Set(Nan::New("[object myObject numberOfStones=4]").ToLocalChecked());
}

void initModule(v8::Local<v8::Object> exports) {
  Nan::Export(exports, "helloWorld", HelloWorld);
  Nan::Export(exports, "returnValueToJS", ReturnValueToJS);
  Nan::Export(exports, "returnObjectToJS", ReturnObjectToJS);
  Nan::Export(exports, "getArgFromJS", GetArgFromJS);
  Nan::Export(exports, "consumeArrayFromJS", ConsumeArrayFromJS);
  Nan::Export(exports, "extractMemberInObject", ExtractMemberInObject);
  Nan::Export(exports, "invokeJSFunc", InvokeJSFunc);
  Nan::Export(exports, "returnPromiseToJS", ReturnPromiseToJS);
  Nan::Export(exports, "waitInCpp", WaitInCpp);

  MyObject::Init(exports);
}

NODE_MODULE(exampleAddon, initModule);
