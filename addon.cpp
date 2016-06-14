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




////////////////////////////////////////////////////////////////////////////////////////



void HelloWorldPromise(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	//auto promise = Promise::Resolver::New(isolate);
	v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(isolate);
	auto promise = resolver->GetPromise();
	//args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, "Hello from librealsense"));
	args.GetReturnValue().Set(promise);

  // // Test directly resolve/reject
	// resolver->Resolve(v8::String::NewFromUtf8(isolate, "Hello from librealsense"));
	// resolver->Reject(v8::String::NewFromUtf8(isolate, "Exception from librealsense"));
}


// Promise returned function







class MyObject : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);

 private:
  explicit MyObject();
  ~MyObject();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void StartThread(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void StopThread(const Nan::FunctionCallbackInfo<v8::Value>& info);

  static Nan::Persistent<v8::Function> constructor;
};


Nan::Persistent<v8::Function> MyObject::constructor;

MyObject::MyObject()
{
}

MyObject::~MyObject() {
}

void MyObject::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("MyObject").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "start", StartThread);
  Nan::SetPrototypeMethod(tpl, "stop", StopThread);

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



static uv_thread_t thread_id;
static uv_async_t async;
static uv_mutex_t mutex;
// Nan::Persistent<v8::Function> startCallback;
Nan::Persistent<v8::Object> thisObject;
bool running = true;

// A event handler which will act on the uvasync_t event on the v8 main thread:
void handle_async_send(uv_async_t *handle) {
  int counter = reinterpret_cast<unsigned long>(handle->data);
  Nan::HandleScope scope;

  const int argc = 2;
  v8::Local<v8::Value> argv[argc] = {
    Nan::New("assOnFire").ToLocalChecked(),
    Nan::New(counter)
  };

  v8::Local<v8::Object> localThisObject = Nan::New(thisObject);

  Nan::MakeCallback(localThisObject, "emit", argc, argv);
}

static void thread_main(void* arg) {
  printf("Log from C++: worker thread started\n");

  async.data = nullptr;
  uv_async_send(&async);

  while (running)
  {
      // That wasn't really that long of an operation, so lets pretend it took longer...
      // Not available on Mac
      std::this_thread::sleep_for(std::chrono::seconds(1));
      // sleep(1);

      unsigned long counter = reinterpret_cast<unsigned long>(async.data);
      ++ counter;
      async.data = reinterpret_cast<void*>(counter);

      uv_async_send(&async);
  }

  printf("Log from C++: worker thread ends.\n");
}

// void StartThread (const v8::FunctionCallbackInfo<v8::Value>& info) {
NAN_METHOD(MyObject::StartThread) {
  Nan::HandleScope scope;

  // startCallback.Reset(info[0].As<v8::Function>());
  thisObject.Reset(info.This());

  uv_async_init(uv_default_loop(), &async, handle_async_send);

  uv_thread_create(&thread_id, thread_main, NULL);

  uv_mutex_init(&mutex);

  info.GetReturnValue().Set(Nan::Undefined());
}

// void StopThread (const v8::FunctionCallbackInfo<v8::Value>& info) {
NAN_METHOD(MyObject::StopThread) {
  Nan::HandleScope scope;
  // v8::Local<v8::Function> callbackHandle = info[0].As<v8::Function>();

  running = false;
  uv_thread_join(&thread_id);

  uv_mutex_destroy(&mutex);

  uv_close((uv_handle_t*) &async, NULL);

  // NanMakeCallback(NanGetCurrentContext()->Global(), callbackHandle, 0, NULL);
  info.GetReturnValue().Set(Nan::Undefined());
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
