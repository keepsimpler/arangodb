////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_V8_SERVER_APPLICATION_V8_H
#define ARANGOD_V8_SERVER_APPLICATION_V8_H 1

#include "ApplicationServer/ApplicationFeature.h"

#include <v8.h>

#include "Basics/ConditionVariable.h"
#include "V8/JSLoader.h"

struct TRI_server_t;
struct TRI_vocbase_t;

namespace arangodb {
namespace aql {
class QueryRegistry;
}

namespace basics {
class Thread;
}

namespace rest {
class HttpRequest;
class ApplicationDispatcher;
class ApplicationScheduler;
}

class GlobalContextMethods {
 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief method type
  //////////////////////////////////////////////////////////////////////////////

  enum MethodType {
    TYPE_UNKNOWN = 0,
    TYPE_RELOAD_ROUTING,
    TYPE_RELOAD_AQL,
    TYPE_COLLECT_GARBAGE,
    TYPE_BOOTSTRAP_COORDINATOR,
    TYPE_WARMUP_EXPORTS
  };

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get a method type number from a type string
  //////////////////////////////////////////////////////////////////////////////

  static MethodType getType(std::string const& type) {
    if (type == "reloadRouting") {
      return TYPE_RELOAD_ROUTING;
    }
    if (type == "reloadAql") {
      return TYPE_RELOAD_AQL;
    }
    if (type == "collectGarbage") {
      return TYPE_COLLECT_GARBAGE;
    }
    if (type == "bootstrapCoordinator") {
      return TYPE_BOOTSTRAP_COORDINATOR;
    }
    if (type == "warmupExports") {
      return TYPE_WARMUP_EXPORTS;
    }

    return TYPE_UNKNOWN;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get name for a method
  //////////////////////////////////////////////////////////////////////////////

  static std::string const getName(MethodType type) {
    switch (type) {
      case TYPE_RELOAD_ROUTING:
        return "reloadRouting";
      case TYPE_RELOAD_AQL:
        return "reloadAql";
      case TYPE_COLLECT_GARBAGE:
        return "collectGarbage";
      case TYPE_BOOTSTRAP_COORDINATOR:
        return "bootstrapCoordinator";
      case TYPE_WARMUP_EXPORTS:
        return "warmupExports";
      case TYPE_UNKNOWN:
      default:
        return "unknown";
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get code for a method
  //////////////////////////////////////////////////////////////////////////////

  static char const* getCode(MethodType type) {
    switch (type) {
      case TYPE_RELOAD_ROUTING:
        return CodeReloadRouting;
      case TYPE_RELOAD_AQL:
        return CodeReloadAql;
      case TYPE_COLLECT_GARBAGE:
        return CodeCollectGarbage;
      case TYPE_BOOTSTRAP_COORDINATOR:
        return CodeBootstrapCoordinator;
      case TYPE_WARMUP_EXPORTS:
        return CodeWarmupExports;
      case TYPE_UNKNOWN:
      default:
        return "";
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief static strings with the code for each method
  //////////////////////////////////////////////////////////////////////////////

  static char const* CodeReloadRouting;
  static char const* CodeReloadAql;
  static char const* CodeCollectGarbage;
  static char const* CodeBootstrapCoordinator;
  static char const* CodeWarmupExports;
};

////////////////////////////////////////////////////////////////////////////////
/// @brief a buffer allocator used for V8
////////////////////////////////////////////////////////////////////////////////

class BufferAllocator : public v8::ArrayBuffer::Allocator {
 public:
  virtual void* Allocate(size_t length) {
    void* data = AllocateUninitialized(length);
    if (data != nullptr) {
      memset(data, 0, length);
    }
    return data;
  }
  virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
  virtual void Free(void* data, size_t) {
    if (data != nullptr) {
      free(data);
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
/// @brief application simple user and session management feature
////////////////////////////////////////////////////////////////////////////////

class ApplicationV8 : public rest::ApplicationFeature {
  ApplicationV8(ApplicationV8 const&) = delete;
  ApplicationV8& operator=(ApplicationV8 const&) = delete;

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief V8 isolate and context
  //////////////////////////////////////////////////////////////////////////////

  struct V8Context {
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief identifier
    ////////////////////////////////////////////////////////////////////////////////

    size_t _id;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief V8 context
    ////////////////////////////////////////////////////////////////////////////////

    v8::Persistent<v8::Context> _context;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief V8 isolate, do not put an underscore in front, because then a lot
    /// of macros stop working!
    ////////////////////////////////////////////////////////////////////////////////

    v8::Isolate* isolate = nullptr;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief V8 locker
    ////////////////////////////////////////////////////////////////////////////////

    v8::Locker* _locker = nullptr;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief adds a global method
    ///
    /// Caller must hold the _contextCondition.
    ////////////////////////////////////////////////////////////////////////////////

    bool addGlobalContextMethod(std::string const&);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief executes all global methods
    ///
    /// Caller must hold the _contextCondition.
    ////////////////////////////////////////////////////////////////////////////////

    void handleGlobalContextMethods();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief executes the cancelation cleanup
    ////////////////////////////////////////////////////////////////////////////////

    void handleCancelationCleanup();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief mutex to protect _globalMethods
    ////////////////////////////////////////////////////////////////////////////////

    Mutex _globalMethodsLock;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief open global methods
    ////////////////////////////////////////////////////////////////////////////////

    std::vector<GlobalContextMethods::MethodType> _globalMethods;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief number of executions since last GC of the context
    ////////////////////////////////////////////////////////////////////////////////

    size_t _numExecutions;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief timestamp of last GC for the context
    ////////////////////////////////////////////////////////////////////////////////

    double _lastGcStamp;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief whether or not the context has dead (ex-v8 wrapped) objects
    ////////////////////////////////////////////////////////////////////////////////

    bool _hasActiveExternals;
  };

 public:
  ApplicationV8(TRI_server_t*, arangodb::aql::QueryRegistry*,
                rest::ApplicationScheduler*, rest::ApplicationDispatcher*);

  ~ApplicationV8();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the app-path
  //////////////////////////////////////////////////////////////////////////////

  std::string const& appPath() const { return _appPath; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief sets the concurrency
  //////////////////////////////////////////////////////////////////////////////

  void setConcurrency(size_t);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief sets the database
  //////////////////////////////////////////////////////////////////////////////

  void setVocbase(TRI_vocbase_t*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief enters an context
  //////////////////////////////////////////////////////////////////////////////

  V8Context* enterContext(TRI_vocbase_t*, bool useDatabase,
                          ssize_t forceContext = -1);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief exists an context
  //////////////////////////////////////////////////////////////////////////////

  void exitContext(V8Context*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief adds a global context function to be executed asap
  //////////////////////////////////////////////////////////////////////////////

  bool addGlobalContextMethod(std::string const&);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief runs the garbage collection
  //////////////////////////////////////////////////////////////////////////////

  void collectGarbage();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief disables actions
  //////////////////////////////////////////////////////////////////////////////

  void disableActions();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief defines a boolean variable
  //////////////////////////////////////////////////////////////////////////////

  void defineBoolean(std::string const& name, bool value) {
    _definedBooleans[name] = value;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief defines a double constant
  //////////////////////////////////////////////////////////////////////////////

  void defineDouble(std::string const& name, double value) {
    _definedDoubles[name] = value;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief upgrades the database
  //////////////////////////////////////////////////////////////////////////////

  void upgradeDatabase(bool skip, bool perform);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief runs the version check
  //////////////////////////////////////////////////////////////////////////////

  void versionCheck();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief prepares the server
  //////////////////////////////////////////////////////////////////////////////

  void prepareServer();

 public:
  void setupOptions(std::map<std::string, basics::ProgramOptionsDescription>&) override final;

  bool prepare() override final;

  bool prepare2() override final;

  bool start() override final;

  void close() override final;

  void stop() override final;

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief determine which of the free contexts should be picked for the GC
  //////////////////////////////////////////////////////////////////////////////

  V8Context* pickFreeContextForGc();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief prepares a V8 instance
  //////////////////////////////////////////////////////////////////////////////

  bool prepareV8Instance(size_t i, bool useActions);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief prepares a V8 instance, multi-threaded version calling the above
  //////////////////////////////////////////////////////////////////////////////

  void prepareV8InstanceInThread(size_t i, bool useAction);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief prepares the V8 server
  //////////////////////////////////////////////////////////////////////////////

  void prepareV8Server(size_t i, std::string const& startupFile);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief shuts down a V8 instance
  //////////////////////////////////////////////////////////////////////////////

  void shutdownV8Instance(size_t i);

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief server object
  //////////////////////////////////////////////////////////////////////////////

  TRI_server_t* _server;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief query registry object
  //////////////////////////////////////////////////////////////////////////////

  arangodb::aql::QueryRegistry* _queryRegistry;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief a buffer allocator for V8
  //////////////////////////////////////////////////////////////////////////////

  BufferAllocator _bufferAllocator;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief path to the directory containing the startup scripts
  ///
  /// `--javascript.startup-directory directory`
  ///
  /// Specifies the *directory* path to the JavaScript files used for
  /// bootstraping.
  //////////////////////////////////////////////////////////////////////////////

  std::string _startupPath;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief semicolon separated list of application directories
  /// `--javascript.app-path directory`
  ///
  /// Specifies the *directory* path where the applications are located.
  /// Multiple paths can be specified separated with commas.
  //////////////////////////////////////////////////////////////////////////////

  std::string _appPath;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief only used for backwards compatibility
  //////////////////////////////////////////////////////////////////////////////

  std::string _devAppPath;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief use actions
  //////////////////////////////////////////////////////////////////////////////

  bool _useActions;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief enables frontend version check
  //////////////////////////////////////////////////////////////////////////////

  bool _frontendVersionCheck;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief was docuBlock jsStartupGcInterval
  ////////////////////////////////////////////////////////////////////////////////

  uint64_t _gcInterval;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief was docuBlock jsGcFrequency
  ////////////////////////////////////////////////////////////////////////////////

  double _gcFrequency;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief was docuBlock jsV8Options
  ////////////////////////////////////////////////////////////////////////////////

  std::string _v8Options;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief V8 startup loader
  //////////////////////////////////////////////////////////////////////////////

  JSLoader _startupLoader;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief system database
  //////////////////////////////////////////////////////////////////////////////

  TRI_vocbase_t* _vocbase;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief number of instances to create
  //////////////////////////////////////////////////////////////////////////////

  size_t _nrInstances;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief V8 contexts
  //////////////////////////////////////////////////////////////////////////////

  V8Context** _contexts;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief V8 contexts queue lock
  //////////////////////////////////////////////////////////////////////////////

  basics::ConditionVariable _contextCondition;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief V8 free contexts
  //////////////////////////////////////////////////////////////////////////////

  std::vector<V8Context*> _freeContexts;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief V8 dirty contexts
  //////////////////////////////////////////////////////////////////////////////

  std::vector<V8Context*> _dirtyContexts;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief V8 busy contexts
  //////////////////////////////////////////////////////////////////////////////

  std::unordered_set<V8Context*> _busyContexts;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief shutdown in progress
  //////////////////////////////////////////////////////////////////////////////

  std::atomic<bool> _stopping;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief garbage collection thread
  //////////////////////////////////////////////////////////////////////////////

  basics::Thread* _gcThread;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief scheduler
  //////////////////////////////////////////////////////////////////////////////

  rest::ApplicationScheduler* _scheduler;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief dispatcher
  //////////////////////////////////////////////////////////////////////////////

  rest::ApplicationDispatcher* _dispatcher;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief boolean to be defined
  //////////////////////////////////////////////////////////////////////////////

  std::map<std::string, bool> _definedBooleans;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief global doubles
  //////////////////////////////////////////////////////////////////////////////

  std::map<std::string, double> _definedDoubles;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief startup file
  //////////////////////////////////////////////////////////////////////////////

  std::string _startupFile;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief indicates whether gc thread is done
  //////////////////////////////////////////////////////////////////////////////

  std::atomic<bool> _gcFinished;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief the V8 platform object
  //////////////////////////////////////////////////////////////////////////////

  v8::Platform* _platform;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief atomic flag to report success of multi-threaded startup
  //////////////////////////////////////////////////////////////////////////////

  std::atomic<bool> _ok;
};
}

#endif
