/*jshint globalstrict:false, strict:false, unused: false */
/*global assertEqual, assertTrue, arango, ARGUMENTS */

////////////////////////////////////////////////////////////////////////////////
/// @brief test the replication
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2010-2012 triagens GmbH, Cologne, Germany
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
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Copyright 2017, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

var jsunity = require("jsunity");
var arangodb = require("@arangodb");
var db = arangodb.db;

var replication = require("@arangodb/replication");
var console = require("console");
var internal = require("internal");
var masterEndpoint = arango.getEndpoint();
var slaveEndpoint = ARGUMENTS[0];

////////////////////////////////////////////////////////////////////////////////
/// @brief test suite
////////////////////////////////////////////////////////////////////////////////

function ReplicationSuite() {
  'use strict';
  var cn = "UnitTestsReplication";

  var connectToMaster = function() {
    arango.reconnect(masterEndpoint, db._name(), "root", "");
    db._flushCache();
  };

  var connectToSlave = function() {
    arango.reconnect(slaveEndpoint, db._name(), "root", "");
    db._flushCache();
  };

  var collectionChecksum = function(name) {
    return db._collection(name).checksum(false, true).checksum;
  };

  var collectionCount = function(name) {
    return db._collection(name).count();
  };

  var compareTicks = function(l, r) {
    var i;
    if (l === null) {
      l = "0";
    }
    if (r === null) {
      r = "0";
    }
    if (l.length !== r.length) {
      return l.length - r.length < 0 ? -1 : 1;
    }

    // length is equal
    for (i = 0; i < l.length; ++i) {
      if (l[i] !== r[i]) {
        return l[i] < r[i] ? -1 : 1;
      }
    }

    return 0;
  };

  var compare = function(masterFunc, masterFunc2, slaveFuncFinal) {
    var state = {};

    assertEqual(cn, db._name());
    db._flushCache();
    masterFunc(state);
    
    connectToSlave();
    assertEqual(cn, db._name());

    var syncResult = replication.sync({
      endpoint: masterEndpoint,
      username: "root",
      password: "",
      verbose: true,
      includeSystem: false,
      keepBarrier: true,
      requireFromPresent: true,
      incremental: true
    });

    assertTrue(syncResult.hasOwnProperty('lastLogTick'));

    connectToMaster();
    masterFunc2(state);

    // use lastLogTick as of now
    state.lastLogTick = replication.logger.state().state.lastLogTick;

    let applierConfiguration = {
      endpoint: masterEndpoint,
      username: "root",
      password: "" 
    };

    connectToSlave();
    assertEqual(cn, db._name());

    replication.applier.properties(applierConfiguration);
    replication.applier.start(syncResult.lastLogTick, syncResult.barrierId);

    var printed = false;

    while (true) {
      var slaveState = replication.applier.state();

      if (slaveState.state.lastError.errorNum > 0) {
        console.log("slave has errored:", JSON.stringify(slaveState.state.lastError));
        break;
      }

      if (!slaveState.state.running) {
        console.log("slave is not running");
        break;
      }

      if (compareTicks(slaveState.state.lastAppliedContinuousTick, state.lastLogTick) >= 0 ||
          compareTicks(slaveState.state.lastProcessedContinuousTick, state.lastLogTick) >= 0) { // ||
       //          compareTicks(slaveState.state.lastAvailableContinuousTick, syncResult.lastLogTick) > 0) {
        console.log("slave has caught up. state.lastLogTick:", state.lastLogTick, "slaveState.lastAppliedContinuousTick:", slaveState.state.lastAppliedContinuousTick, "slaveState.lastProcessedContinuousTick:", slaveState.state.lastProcessedContinuousTick);
        break;
      }
        
      if (!printed) {
        console.log("waiting for slave to catch up");
        printed = true;
      }
      internal.wait(0.5, false);
    }

    db._flushCache();
    slaveFuncFinal(state);
  };

  return {

    setUp: function() {
      db._useDatabase("_system");
      connectToMaster();
      try {
        db._dropDatabase(cn);
      } catch (err) {}

      db._createDatabase(cn);
      db._useDatabase(cn);

      db._useDatabase("_system");
      connectToSlave();
      
      try {
        db._dropDatabase(cn);
      } catch (err) {}

      db._createDatabase(cn);
    },

    tearDown: function() {
      db._useDatabase("_system");
      connectToMaster();

      db._useDatabase(cn);
      connectToSlave();
      replication.applier.stop();
      replication.applier.forget();
      
      db._useDatabase("_system");
      db._dropDatabase(cn);
    },
    
    testFuzz: function() {
      db._useDatabase(cn);
      connectToMaster();

      compare(
        function(state) {
        },

        function(state) {
          let pickDatabase = function() {
            db._useDatabase('_system');
            let dbs;
            while (true) {
              dbs = db._databases().filter(function(db) { return db !== '_system'; });
              if (dbs.length !== 0) {
                break;
              }
              createDatabase();
            }
            let d = dbs[Math.floor(Math.random() * dbs.length)];
            db._useDatabase(d);
          };
          
          let pickCollection = function() {
            let collections;
            while (true) {
              collections = db._collections().filter(function(c) { return c.name()[0] !== '_' && c.type() === 2; });
              if (collections.length !== 0) {
                break;
              }
              return createCollection();
            }
            return collections[Math.floor(Math.random() * collections.length)];
          };
          
          let pickEdgeCollection = function() {
            let collections;
            while (true) {
              collections = db._collections().filter(function(c) { return c.name()[0] !== '_' && c.type() === 3; });
              if (collections.length !== 0) {
                break;
              }
              return createEdgeCollection();
            }
            return collections[Math.floor(Math.random() * collections.length)];
          };

          let insert = function() {
            let collection = pickCollection();
            collection.insert({ value: Date.now() });
          };
          
          let remove = function() {
            let collection = pickCollection();
            if (collection.count() === 0) {
              let k = collection.insert({});
              collection.remove(k);
              return;
            }
            collection.remove(collection.any());
          };

          let replace = function() {
            let collection = pickCollection();
            if (collection.count() === 0) {
              let k = collection.insert({});
              collection.replace(k, { value2: Date.now() });
              return;
            }
            collection.replace(collection.any(), { value2: Date.now() });
          };
          
          let update = function() {
            let collection = pickCollection();
            if (collection.count() === 0) {
              let k = collection.insert({});
              collection.update(k, { value2: Date.now() });
              return;
            }
            collection.update(collection.any(), { value2: Date.now() });
          };
          
          let insertEdge = function() {
            let collection = pickEdgeCollection();
            collection.insert({ _from: "test/v1", _to: "test/v2", value: Date.now() });
          };
          
          let insertOrReplace = function() {
            let collection = pickCollection();
            db._executeTransaction({
              collections: { write: [collection.name()] },
              action: function(params) {
                let collection = params.cn, db = require("internal").db;
                let key = "test" + Math.floor(Math.random() * 10000);
                try {
                  db[collection].insert({ _key: key, value: Date.now() });
                } catch (err) {
                  db[collection].replace(key, { value2: Date.now() });
                }
              },
              params: { cn: collection.name() }
            });
          };
          
          let insertOrUpdate = function() {
            let collection = pickCollection();
            db._executeTransaction({
              collections: { write: [collection.name()] },
              action: function(params) {
                let collection = params.cn, db = require("internal").db;
                let key = "test" + Math.floor(Math.random() * 10000);
                try {
                  db[collection].insert({ _key: key, value: Date.now() });
                } catch (err) {
                  db[collection].update(key, { value2: Date.now() });
                }
              },
              params: { cn: collection.name() }
            });
          };
          
          let insertMulti = function() {
            let collection = pickCollection();
            db._executeTransaction({
              collections: { write: [collection.name()] },
              action: function(params) {
                let collection = params.cn, db = require("internal").db;
                db[collection].insert({ value1: Date.now() });
                db[collection].insert({ value2: Date.now() });
              },
              params: { cn: collection.name() }
            });
          };
          
          let removeMulti = function() {
            let collection = pickCollection();
            db._executeTransaction({
              collections: { write: [collection.name()] },
              action: function(params) {
                let collection = params.cn, db = require("internal").db;
                if (db[collection].count() < 2) {
                  let k1 = db[collection].insert({});
                  let k2 = db[collection].insert({});
                  db[collection].remove(k1);
                  db[collection].remove(k2);
                  return;
                }
                db[collection].remove(db[collection].any());
                db[collection].remove(db[collection].any());
              },
              params: { cn: collection.name() }
            });
          };
          
          let removeInsert = function() {
            let collection = pickCollection();
            db._executeTransaction({
              collections: { write: [collection.name()] },
              action: function(params) {
                let collection = params.cn, db = require("internal").db;
                if (db[collection].count() === 0) {
                  db[collection].insert({ value: Date.now() });
                }
                db[collection].remove(db[collection].any());
                db[collection].insert({ value: Date.now() });
              },
              params: { cn: collection.name() }
            });
          };
          
          let insertRemove = function() {
            let collection = pickCollection();
            db._executeTransaction({
              collections: { write: [collection.name()] },
              action: function(params) {
                let collection = params.cn, db = require("internal").db;
                let k = db[collection].insert({ value: Date.now() });
                db[collection].remove(k);
              },
              params: { cn: collection.name() }
            });
          };
          
          let createCollection = function() {
            let name = "test" + internal.genRandomAlphaNumbers(16) + Date.now();
            return db._create(name);
          };
          
          let createEdgeCollection = function() {
            let name = "edge" + internal.genRandomAlphaNumbers(16) + Date.now();
            return db._createEdgeCollection(name);
          };
          
          let dropCollection = function() {
            let collection = pickCollection();
            if (db._name() === cn) {
              // don't drop collections from our test database
              return false;
            }
            db._drop(collection.name());
          };
          
          let renameCollection = function() {
            let name = internal.genRandomAlphaNumbers(16) + Date.now();
            let collection = pickCollection();
            collection.rename("fuchs" + name);
          };
          
          let changeCollection = function() {
            let collection = pickCollection();
            collection.properties({ waitForSync: false });
          };

          let createIndex = function () {
            let name = internal.genRandomAlphaNumbers(16) + Date.now();
            let collection = pickCollection();
            collection.ensureIndex({ 
              type: Math.random() >= 0.5 ? "hash" : "skiplist", 
              fields: [ name ],
              sparse: Math.random() > 0.5 
            }); 
          };

          let dropIndex = function () {
            let collection = pickCollection();
            let indexes = collection.getIndexes();
            if (indexes.length > 1) {
              collection.dropIndex(indexes[1]);
            }
          };

          let createDatabase = function() {
            db._useDatabase('_system');
            let name = "test" + internal.genRandomAlphaNumbers(16) + Date.now();
            return db._createDatabase(name);
          };

          let dropDatabase = function () {
            pickDatabase();
            let name = db._name();
            if (name === cn) {
              // we do not want to delete our test database
              return;
            }
            db._useDatabase('_system');
            db._dropDatabase(name);
          };


          let ops = [
            { name: "insert", func: insert },
            { name: "remove", func: remove },
            { name: "replace", func: replace },
            { name: "update", func: update },
            { name: "insertEdge", func: insertEdge },
            { name: "insertOrReplace", func: insertOrReplace },
            { name: "insertOrUpdate", func: insertOrUpdate },
            { name: "insertMulti", func: insertMulti },
            { name: "removeMulti", func: removeMulti },
            { name: "removeInsert", func: removeInsert },
            { name: "insertRemove", func: insertRemove },
            { name: "createCollection", func: createCollection },
            { name: "dropCollection", func: dropCollection },
            { name: "renameCollection", func: renameCollection },
            { name: "changeCollection", func: changeCollection },
            { name: "createIndex", func: createIndex },
            { name: "dropIndex", func: dropIndex },
            { name: "createDatabase", func: createDatabase },
            { name: "dropDatabase", func: dropDatabase },
          ];
     
          for (let i = 0; i < 3000; ++i) { 
            pickDatabase();
            let op = ops[Math.floor(Math.random() * ops.length)];
            op.func();
          }

          db._useDatabase(cn);
          let total = "";
          db._collections().filter(function(c) { return c.name()[0] !== '_'; }).forEach(function(c) {
            total += c.name() + "-" + c.count() + "-" + collectionChecksum(c.name());
          });
          state.state = total;
        },

        function(state) {
          db._useDatabase(cn);
          let total = "";
          db._collections().filter(function(c) { return c.name()[0] !== '_'; }).forEach(function(c) {
            total += c.name() + "-" + c.count() + "-" + collectionChecksum(c.name());
          });
          assertTrue(total.length > 0);
          assertEqual(total, state.state);
        }
      );
    }

  };
}

////////////////////////////////////////////////////////////////////////////////
/// @brief executes the test suite
////////////////////////////////////////////////////////////////////////////////

jsunity.run(ReplicationSuite);

return jsunity.done();
