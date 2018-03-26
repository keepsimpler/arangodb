////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2018 ArangoDB GmbH, Cologne, Germany
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
/// @author Tobias Gödderz
////////////////////////////////////////////////////////////////////////////////

#include "ClusterRepairOperations.h"
#include "ServerState.h"

using namespace arangodb;
using namespace arangodb::basics;
using namespace arangodb::velocypack;
using namespace arangodb::cluster_repairs;



bool cluster_repairs::operator==(BeginRepairsOperation const &left, BeginRepairsOperation const &right) {
  return
    left.database == right.database
    && left.collectionId == right.collectionId
    && left.collectionName == right.collectionName
    && left.protoCollectionId == right.protoCollectionId
    && left.protoCollectionName == right.protoCollectionName
    && left.collectionReplicationFactor == right.collectionReplicationFactor
    && left.protoReplicationFactor == right.protoReplicationFactor
    && left.renameDistributeShardsLike == right.renameDistributeShardsLike;
}


bool cluster_repairs::operator==(FinishRepairsOperation const &left, FinishRepairsOperation const &right) {
  return
    left.database == right.database
    && left.collectionId == right.collectionId
    && left.collectionName == right.collectionName
    && left.protoCollectionId == right.protoCollectionId
    && left.protoCollectionName == right.protoCollectionName
    && left.shards == right.shards
    && left.replicationFactor == right.replicationFactor;
}


bool cluster_repairs::operator==(MoveShardOperation const &left, MoveShardOperation const &right) {
  return
    left.database == right.database
    && left.collectionId == right.collectionId
    && left.collectionName == right.collectionName
    && left.shard == right.shard
    && left.from == right.from
    && left.to == right.to
    && left.isLeader == right.isLeader;
}


bool cluster_repairs::operator==(FixServerOrderOperation const &left, FixServerOrderOperation const &right) {
  return
    left.database == right.database
    && left.collectionId == right.collectionId
    && left.collectionName == right.collectionName
    && left.protoCollectionId == right.protoCollectionId
    && left.protoCollectionName == right.protoCollectionName
    && left.shard == right.shard
    && left.protoShard == right.protoShard
    && left.leader == right.leader
    && left.followers == right.followers
    && left.protoFollowers == right.protoFollowers;
}



std::ostream& cluster_repairs::operator<<(std::ostream& ostream, BeginRepairsOperation const& operation) {
  ostream
    << "BeginRepairsOperation" << std::endl
    << "{ database: " << operation.database << std::endl
    << ", collection: " << operation.collectionName << " (" << operation.collectionId << ")" << std::endl
    << ", protoCollection: " << operation.protoCollectionName << " (" << operation.protoCollectionId << ")" << std::endl
    << ", collectionReplicationFactor: " << operation.collectionReplicationFactor << std::endl
    << ", protoReplicationFactor: " << operation.protoReplicationFactor << std::endl
    << ", renameDistributeShardsLike: " << operation.renameDistributeShardsLike << std::endl
    << "}";

  return ostream;
}


std::ostream& cluster_repairs::operator<<(std::ostream& ostream, FinishRepairsOperation const& operation) {

  auto printShard = [](std::ostream& ostream, ShardWithProtoAndDbServers shardWithProtoAndDbServers) {
    ShardID shardId, protoShardId;
    DBServers dbServers;
    std::tie(shardId, protoShardId, dbServers) = shardWithProtoAndDbServers;

    ostream << "{ ";
    ostream << "shard: " << shardId << ", ";
    ostream << "protoShard: " << protoShardId << ", ";
    ostream << "dbServers: ";

    if (dbServers.empty()) {
      ostream << "[]";
      return;
    }
    else {
      auto it = dbServers.begin();
      ostream << "[" << *it;
      ++it;

      for (; it != dbServers.end(); ++it) {
        ostream << ", " << *it;
      }

      ostream << "]";
    }
    ostream << "}";
  };

  auto printShards = [&printShard](std::ostream& ostream, std::vector<ShardWithProtoAndDbServers> shards) {
    if (shards.empty()) {
      ostream << "  []";
      return;
    }

    auto it = shards.begin();
    ostream << "  ] ";
    printShard(ostream, *it);
    ostream << "\n";
    ++it;

    for(; it != shards.end(); ++it) {
      ostream << "  , ";
      printShard(ostream, *it);
      ostream << "\n";
    }

    ostream << "  [";
  };


  ostream
    << "FinishRepairsOperation" << std::endl
    << "{ database: " << operation.database << std::endl
    << ", collection: " << operation.collectionName << " (" << operation.collectionId << ")" << std::endl
    << ", protoCollection: " << operation.protoCollectionName << " (" << operation.protoCollectionId << ")" << std::endl;

  ostream << ", shards: \n";
  printShards(ostream, operation.shards);
  ostream << std::endl;

  ostream
    << ", replicationFactor: " << operation.replicationFactor << std::endl
    << "}";

  return ostream;
}


std::ostream& cluster_repairs::operator<<(std::ostream& ostream, MoveShardOperation const& operation) {
  ostream
    << "MoveShardOperation" << std::endl
    << "{ database: " << operation.database << std::endl
    << ", collection: " << operation.collectionName << " (" << operation.collectionId << ")" << std::endl
    << ", shard: " << operation.shard << std::endl
    << ", from: " << operation.from << std::endl
    << ", to: " << operation.to << std::endl
    << ", isLeader: " << operation.isLeader << std::endl
    << "}";

  return ostream;
}


std::ostream& cluster_repairs::operator<<(std::ostream& ostream, FixServerOrderOperation const& operation) {
  ostream
    << "FixServerOrderOperation" << std::endl
    << "{ database: " << operation.database << std::endl
    << ", collection: " << operation.collectionName << " (" << operation.collectionId << ")" << std::endl
    << ", protoCollection: " << operation.protoCollectionName << " (" << operation.protoCollectionId << ")" << std::endl
    << ", shard: " << operation.shard << std::endl
    << ", protoShard: " << operation.protoShard << std::endl
    << ", leader: " << operation.leader << std::endl
    << ", followers: [ ";
  for(auto const&it : operation.followers) {
    ostream << it << ", ";
  }
  ostream << "]" << std::endl;

  ostream
    << ", protoFollowers: [ ";
  for(auto const&it : operation.protoFollowers) {
    ostream << it << ", ";
  }
  ostream << "]" << std::endl
          << "}";

  return ostream;
}


class StreamRepairOperationVisitor
  : public boost::static_visitor<std::ostream&>
{
 public:
  StreamRepairOperationVisitor() = delete;
  StreamRepairOperationVisitor(std::ostream& stream_)
    : _stream(stream_) {};

  std::ostream& operator()(BeginRepairsOperation const& op) {
    return _stream << op;
  }
  std::ostream& operator()(FinishRepairsOperation const& op) {
    return _stream << op;
  }
  std::ostream& operator()(MoveShardOperation const& op) {
    return _stream << op;
  }
  std::ostream& operator()(FixServerOrderOperation const& op) {
    return _stream << op;
  }

 private:
  std::ostream &_stream;
};


std::ostream& cluster_repairs::operator<<(std::ostream& ostream, RepairOperation const& operation) {
  auto visitor = StreamRepairOperationVisitor(ostream);
  return boost::apply_visitor(visitor, operation);
}

std::string
getExtendedIsoString(std::chrono::system_clock::time_point time_point) {
  std::time_t time_t = std::chrono::system_clock::to_time_t(time_point);
  char const* format = "%FT%TZ";
  char timeString[100];
  size_t bytesWritten
    = std::strftime(timeString, sizeof(timeString), format, std::gmtime(&time_t));

  TRI_ASSERT(bytesWritten > 0);
  if (bytesWritten == 0) {
    // This should never happen
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_FAILED,
      "strftime returned 0 - buffer too small?");
  }

  return std::string(timeString);
}

class RepairOperationTypeStringVisitor
  : public boost::static_visitor<std::string>
{
 public:
  std::string operator()(BeginRepairsOperation const& op) const {
    return "BeginRepairsOperation";
  }
  std::string operator()(FinishRepairsOperation const& op) const {
    return "FinishRepairsOperation";
  }
  std::string operator()(MoveShardOperation const& op) const {
    return "MoveShardOperation";
  }
  std::string operator()(FixServerOrderOperation const& op) const {
    return "FixServerOrderOperation";
  }
};


std::string
cluster_repairs::getTypeAsString(RepairOperation const& op) {
  return boost::apply_visitor(RepairOperationTypeStringVisitor(), op);
}


VPackBufferPtr
MoveShardOperation::toVpackTodo(
  uint64_t jobId,
  std::chrono::system_clock::time_point jobCreationTimestamp
) const {
  std::string const serverId = ServerState::instance()->getId();

  std::string const isoTimeString
    = getExtendedIsoString(jobCreationTimestamp);

  VPackBuilder builder;

  builder.add(VPackValue(VPackValueType::Object));

  builder.add("type", VPackValue("moveShard"));
  builder.add("database", VPackValue(database));
  builder.add("collection", VPackValue(collectionId));
  builder.add("shard", VPackValue(shard));
  builder.add("fromServer", VPackValue(from));
  builder.add("toServer", VPackValue(to));
  builder.add("jobId", VPackValue(std::to_string(jobId)));
  builder.add("timeCreated", VPackValue(isoTimeString));
  builder.add("creator", VPackValue(serverId));
  builder.add("isLeader", VPackValue(isLeader));

  builder.close();

  return builder.steal();
}


RepairOperationToTransactionVisitor::ReturnValueT
RepairOperationToTransactionVisitor::operator()(BeginRepairsOperation const& op) {
  std::string const distributeShardsLikePath
    = agencyCollectionId(op.database, op.collectionId)
      + "/" + "distributeShardsLike";
  std::string const repairingDistributeShardsLikePath
    = agencyCollectionId(op.database, op.collectionId)
      + "/" + "repairingDistributeShardsLike";
  std::string const replicationFactorPath
    = agencyCollectionId(op.database, op.collectionId)
      + "/" + "replicationFactor";
  std::string const protoReplicationFactorPath
    = agencyCollectionId(op.database, op.protoCollectionId)
      + "/" + "replicationFactor";

  VPackBuilder builder;
  builder.add(VPackValue(op.protoCollectionId));
  velocypack::Slice protoCollectionIdSlice = builder.slice();
  _vpackBufferArray.emplace_back(std::move(builder.steal()));

  builder = VPackBuilder();
  builder.add(VPackValue(op.collectionReplicationFactor));
  velocypack::Slice collectionReplicationFactorSlice = builder.slice();
  _vpackBufferArray.emplace_back(std::move(builder.steal()));

  builder = VPackBuilder();
  builder.add(VPackValue(op.protoReplicationFactor));
  velocypack::Slice protoReplicationFactorSlice = builder.slice();
  _vpackBufferArray.emplace_back(std::move(builder.steal()));

  std::vector<AgencyPrecondition> preconditions;

  if (op.renameDistributeShardsLike) {
    // assert that distributeShardsLike is set, but repairingDistributeShardsLike
    // is not
    preconditions.emplace_back(
      AgencyPrecondition {
        distributeShardsLikePath,
        AgencyPrecondition::Type::VALUE,
        protoCollectionIdSlice,
      }
    );
    preconditions.emplace_back(
      AgencyPrecondition {
        repairingDistributeShardsLikePath,
        AgencyPrecondition::Type::EMPTY,
        true,
      }
    );
  }
  else {
    // assert that repairingDistributeShardsLike is set, but distributeShardsLike
    // is not
    preconditions.emplace_back(
      AgencyPrecondition {
        repairingDistributeShardsLikePath,
        AgencyPrecondition::Type::VALUE,
        protoCollectionIdSlice,
      }
    );
    preconditions.emplace_back(
      AgencyPrecondition {
        distributeShardsLikePath,
        AgencyPrecondition::Type::EMPTY,
        true,
      }
    );
  }

  // assert replicationFactors
  preconditions.emplace_back(
    AgencyPrecondition {
      replicationFactorPath,
      AgencyPrecondition::Type::VALUE,
      collectionReplicationFactorSlice,
    }
  );
  preconditions.emplace_back(
    AgencyPrecondition {
      protoReplicationFactorPath,
      AgencyPrecondition::Type::VALUE,
      protoReplicationFactorSlice,
    }
  );

  std::vector<AgencyOperation> operations;
  if (op.renameDistributeShardsLike) {
    operations.emplace_back(
      AgencyOperation {
        repairingDistributeShardsLikePath,
        AgencyValueOperationType::SET,
        protoCollectionIdSlice,
      }
    );
    operations.emplace_back(
      AgencyOperation {
        distributeShardsLikePath,
        AgencySimpleOperationType::DELETE_OP,
      }
    );

    // Fix the replicationFactor to be the same as the proto collection,
    // but only when also renaming distributeShardsLike.
    // Rationale: As long as distributeShardsLike is set,
    // collection.replicationFactor is ignored but proto.replicationFactor
    // is used instead.
    // This way, we never change the replicationFactor that is actually used.
    if (op.collectionReplicationFactor != op.protoReplicationFactor) {
      operations.emplace_back(
        AgencyOperation {
          replicationFactorPath,
          AgencyValueOperationType::SET,
          protoReplicationFactorSlice,
        }
      );
    }
    else {
      preconditions.emplace_back(
        AgencyPrecondition {
          replicationFactorPath,
          AgencyPrecondition::Type::VALUE,
          protoReplicationFactorSlice
        }
      );
    }
  }

  return {
    AgencyWriteTransaction {operations, preconditions},
    boost::none,
  };
}


RepairOperationToTransactionVisitor::ReturnValueT
RepairOperationToTransactionVisitor::operator()(FinishRepairsOperation const& op) {
  std::string const oldAttrPath
    = agencyCollectionId(op.database, op.collectionId)
      + "/" + "repairingDistributeShardsLike";
  std::string const newAttrPath
    = agencyCollectionId(op.database, op.collectionId)
      + "/" + "distributeShardsLike";
  std::string const replicationFactorPath
    = agencyCollectionId(op.database, op.collectionId)
      + "/" + "replicationFactor";
  std::string const protoReplicationFactorPath
    = agencyCollectionId(op.database, op.protoCollectionId)
      + "/" + "replicationFactor";

  VPackBuilder builder;
  builder.add(VPackValue(op.protoCollectionId));
  velocypack::Slice protoCollectionIdSlice = builder.slice();
  _vpackBufferArray.emplace_back(std::move(builder.steal()));

  builder = VPackBuilder();
  builder.add(VPackValue(op.replicationFactor));
  velocypack::Slice replicationFactorSlice = builder.slice();
  _vpackBufferArray.emplace_back(std::move(builder.steal()));

  std::vector<AgencyPrecondition> preconditions{
    AgencyPrecondition {
      oldAttrPath,
      AgencyPrecondition::Type::VALUE,
      protoCollectionIdSlice,
    },
    AgencyPrecondition {
      newAttrPath,
      AgencyPrecondition::Type::EMPTY,
      true,
    },
    AgencyPrecondition {
      replicationFactorPath,
      AgencyPrecondition::Type::VALUE,
      replicationFactorSlice,
    },
    AgencyPrecondition {
      protoReplicationFactorPath,
      AgencyPrecondition::Type::VALUE,
      replicationFactorSlice,
    },
  };

  for(auto const& it : op.shards) {
    ShardID shardId, protoShardId;
    DBServers dbServers;
    std::tie(shardId, protoShardId, dbServers) = it;
    std::string const shardPath
      = agencyCollectionId(op.database, op.collectionId)
        + "/shards/" + shardId;
    std::string const protoShardPath
      = agencyCollectionId(op.database, op.protoCollectionId)
        + "/shards/" + protoShardId;

    VPackSlice dbServersSlice;
    {
      VPackBuilder dbServerBuilder;
      dbServerBuilder.add(VPackValue(velocypack::ValueType::Array));
      for (auto const &dbServer : dbServers) {
        dbServerBuilder.add(VPackValue(dbServer));
      }
      dbServerBuilder.close();

      dbServersSlice = dbServerBuilder.slice();
      _vpackBufferArray.emplace_back(std::move(dbServerBuilder.steal()));
    }

    preconditions.emplace_back(AgencyPrecondition{
      shardPath,
      AgencyPrecondition::Type::VALUE,
      dbServersSlice
    });
    preconditions.emplace_back(AgencyPrecondition{
      protoShardPath,
      AgencyPrecondition::Type::VALUE,
      dbServersSlice
    });
  }

  std::vector<AgencyOperation> operations{
    AgencyOperation {
      newAttrPath,
      AgencyValueOperationType::SET,
      protoCollectionIdSlice
    },
    AgencyOperation {
      oldAttrPath,
      AgencySimpleOperationType::DELETE_OP,
    },
  };

  return {
    AgencyWriteTransaction {operations, preconditions},
    boost::none
  };
}


RepairOperationToTransactionVisitor::ReturnValueT
RepairOperationToTransactionVisitor::operator()(MoveShardOperation const& op) {
  uint64_t jobId = _getJobId();
  std::chrono::system_clock::time_point jobCreationTimestamp
    = _getJobCreationTimestamp();

  VPackBufferPtr vpackTodo = op.toVpackTodo(jobId, jobCreationTimestamp);

  _vpackBufferArray.push_back(vpackTodo);

  std::string const agencyKey = "Target/ToDo/" + std::to_string(jobId);

  return std::make_pair(AgencyWriteTransaction {
    AgencyOperation {
      agencyKey,
      AgencyValueOperationType::SET,
      VPackSlice(vpackTodo->data())
    },
    AgencyPrecondition {
      agencyKey,
      AgencyPrecondition::Type::EMPTY,
      true,
    }
  }, jobId);
}


RepairOperationToTransactionVisitor::ReturnValueT
RepairOperationToTransactionVisitor::operator()(FixServerOrderOperation const& op) {
  std::string const agencyShardId
    = agencyCollectionId(op.database, op.collectionId)
      + "/shards/" + op.shard;
  std::string const agencyProtoShardId
    = agencyCollectionId(op.database, op.protoCollectionId)
      + "/shards/" + op.protoShard;

  VPackBufferPtr vpack = createShardDbServerArray(op.leader, op.followers);
  VPackSlice oldDbServerSlice = velocypack::Slice(vpack->data());
  _vpackBufferArray.emplace_back(std::move(vpack));

  vpack = createShardDbServerArray(op.leader, op.protoFollowers);
  VPackSlice protoDbServerSlice = velocypack::Slice(vpack->data());
  _vpackBufferArray.emplace_back(std::move(vpack));

  std::vector<AgencyPrecondition> agencyPreconditions {
    AgencyPrecondition {
      agencyShardId,
      AgencyPrecondition::Type::VALUE,
      oldDbServerSlice
    },
    AgencyPrecondition {
      agencyProtoShardId,
      AgencyPrecondition::Type::VALUE,
      protoDbServerSlice
    },
  };

  AgencyOperation agencyOperation {
    agencyShardId,
    AgencyValueOperationType::SET,
    protoDbServerSlice
  };

  return {
    AgencyWriteTransaction { agencyOperation, agencyPreconditions },
    boost::none
  };
}


std::string
RepairOperationToTransactionVisitor::agencyCollectionId(
  DatabaseID database,
  CollectionID collection
) const {
  return "Plan/Collections/" + database + "/" + collection;
}


VPackBufferPtr
RepairOperationToTransactionVisitor::createShardDbServerArray(
  ServerID const& leader,
  DBServers const& followers
) const {
  VPackBuilder builder;

  builder.add(velocypack::Value(velocypack::ValueType::Array));

  builder.add(velocypack::Value(leader));

  for (auto const& it : followers) {
    builder.add(velocypack::Value(it));
  }

  builder.close();

  return builder.steal();
}

RepairOperationToTransactionVisitor::RepairOperationToTransactionVisitor()
: _getJobId([]() { return ClusterInfo::instance()->uniqid(); }),
  _getJobCreationTimestamp([]() { return std::chrono::system_clock::now(); })
{ }

RepairOperationToTransactionVisitor::RepairOperationToTransactionVisitor(
  std::function<uint64_t()> getJobId_,
  std::function<std::chrono::system_clock::time_point()> getJobCreationTimestamp_
) : _getJobId(getJobId_),
    _getJobCreationTimestamp(getJobCreationTimestamp_)
{ }

std::vector<VPackBufferPtr>&& RepairOperationToTransactionVisitor::steal() {
  return std::move(_vpackBufferArray);
}

RepairOperationToVPackVisitor::RepairOperationToVPackVisitor(
  VPackBuilder &builder_
) : _builder(builder_) {
}

VPackBuilder&
RepairOperationToVPackVisitor::builder() {
  return _builder;
}

void
RepairOperationToVPackVisitor::operator()(
  BeginRepairsOperation const &op
) {
  VPackObjectBuilder outerObject(&builder());
  {
    VPackObjectBuilder innerObject(&builder(), "BeginRepairsOperation");

    builder().add("database", VPackValue(op.database));
    builder().add("collection", VPackValue(op.collectionName));
    builder().add("distributeShardsLike", VPackValue(op.protoCollectionName));
    builder().add("renameDistributeShardsLike", VPackValue(op.renameDistributeShardsLike));
  }
}

void
RepairOperationToVPackVisitor::operator()(
  FinishRepairsOperation const &op
) {
  VPackObjectBuilder outerObject(&builder());
  {
    VPackObjectBuilder innerObject(&builder(), "FinishRepairsOperation");

    builder().add("database", VPackValue(op.database));
    builder().add("collection", VPackValue(op.collectionName));
    builder().add("distributeShardsLike", VPackValue(op.protoCollectionName));

    VPackArrayBuilder shards(&builder(), "shards");
    for (auto const& it : op.shards) {
      ShardID shardId, protoShardId;
      DBServers dbServers;
      std::tie(shardId, protoShardId, dbServers) = it;
      VPackObjectBuilder shard(&builder());
      builder().add("shard", VPackValue(shardId));
      builder().add("protoShard", VPackValue(protoShardId));
      {
        VPackArrayBuilder dbServersGuard(&builder(), "dbServers");
        for (auto const& dbServer : dbServers) {
          builder().add(VPackValue(dbServer));
        }
      }
    }
  }
}

void
RepairOperationToVPackVisitor::operator()(
  MoveShardOperation const &op
) {
  VPackObjectBuilder outerObject(&builder());
  {
    VPackObjectBuilder innerObject(&builder(), "MoveShardOperation");

    builder().add("database", VPackValue(op.database));
    builder().add("collection", VPackValue(op.collectionName));
    builder().add("shard", VPackValue(op.shard));
    builder().add("from", VPackValue(op.from));
    builder().add("to", VPackValue(op.to));
    builder().add("isLeader", VPackValue(op.isLeader));
  }
}

void
RepairOperationToVPackVisitor::operator()(
  FixServerOrderOperation const &op
) {
  VPackObjectBuilder outerObject(&builder());
  {
    VPackObjectBuilder innerObject(&builder(), "FixServerOrderOperation");
    builder().add("database", VPackValue(op.database));
    builder().add("collection", VPackValue(op.collectionName));
    builder().add("distributeShardsLike", VPackValue(op.protoCollectionName));
    builder().add("shard", VPackValue(op.shard));
    builder().add("distributeShardsLikeShard", VPackValue(op.protoShard));
    builder().add("leader", VPackValue(op.leader));
    {
      VPackArrayBuilder followers(&builder(), "followers");
      for (ServerID const& follower : op.followers) {
        builder().add(VPackValue(follower));
      }
    }
    {
      VPackArrayBuilder distributeShardsLikeFollowers(&builder(), "distributeShardsLikeFollowers");
      for (ServerID const& protoFollower : op.protoFollowers) {
        builder().add(VPackValue(protoFollower));
      }
    }
  }
}
