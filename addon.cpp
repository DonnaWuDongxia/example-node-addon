#include <node.h>
#include <nan.h>

#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <chrono>
#include <tuple>

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
NAN_METHOD(Wait) {
  using ResolverPersistent = Nan::Persistent<v8::Promise::Resolver>;

  auto ms = Nan::To<unsigned>(info[0]).FromJust();
  auto resolver = v8::Promise::Resolver::New(info.GetIsolate());
  auto promise = resolver->GetPromise();
  auto persistent = new ResolverPersistent(resolver);

  uv_timer_t* handle = new uv_timer_t;
  handle->data = persistent;
  uv_timer_init(uv_default_loop(), handle);

  // use capture-less lambda for c-callback
  auto timercb = [](uv_timer_t* handle) -> void {
    Nan::HandleScope scope;

    auto persistent = static_cast<ResolverPersistent*>(handle->data);
    
    // uv_timer_stop(handle);
    // uv_close(reinterpret_cast<uv_handle_t*>(handle),
    //          [](uv_handle_t* handle) -> void {delete handle;});
  
    auto resolver = Nan::New(*persistent);
    resolver->Resolve(Nan::New("'C++ Resolved String Value'").ToLocalChecked());

    persistent->Reset();
    delete persistent;
  };
  uv_timer_start(handle, timercb, ms, 0);

  info.GetReturnValue().Set(promise);
}






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
  Nan::Export(exports, "wait", Wait);
  Nan::Export(exports, "helloWorld", HelloWorld);
  Nan::Export(exports, "returnValueToJS", ReturnValueToJS);
  Nan::Export(exports, "returnObjectToJS", ReturnObjectToJS);
  Nan::Export(exports, "getArgFromJS", GetArgFromJS);

  NODE_SET_METHOD(exports, "helloWorldPromise", HelloWorldPromise);

  MyObject::Init(exports);
}

NODE_MODULE(exampleAddon, initModule);
