// Created by swx on 23-5-30.
#include "Persister.h"
#include "util.h"

void Persister::Save(const std::string raftstate, const std::string snapshot) {
    std::lock_guard<std::mutex> lg(m_mtx);
    clearRaftState(); // 只清空 raftState，因为 raftState 必须更新
    
    // 关键修复：只有 snapshot 非空时才清空并写入快照
    if (!snapshot.empty()) {
        clearSnapshot();
        m_snapshotOutStream << snapshot;
        m_snapshotOutStream.flush(); //  确保快照落盘
    }
    
    m_raftStateOutStream << raftstate;
    m_raftStateOutStream.flush(); //  确保 Raft 状态落盘
}

std::string Persister::ReadSnapshot() {
    std::lock_guard<std::mutex> lg(m_mtx);
    if (m_snapshotOutStream.is_open()) {
        m_snapshotOutStream.close();
    }
    DEFER { m_snapshotOutStream.open(m_snapshotFileName); };
    std::fstream ifs(m_snapshotFileName, std::ios_base::in);
    if (!ifs.good()) {
        return "";
    }
    std::string snapshot;
    ifs >> snapshot;
    ifs.close();
    return snapshot;
}

void Persister::SaveRaftState(const std::string &data) {
    std::lock_guard<std::mutex> lg(m_mtx);
    clearRaftState();
    m_raftStateOutStream << data;
    m_raftStateOutStream.flush(); //  新增 flush
    m_raftStateSize = data.size();
}

long long Persister::RaftStateSize() {
    std::lock_guard<std::mutex> lg(m_mtx);
    return m_raftStateSize;
}

std::string Persister::ReadRaftState() {
    std::lock_guard<std::mutex> lg(m_mtx);
    std::fstream ifs(m_raftStateFileName, std::ios_base::in);
    if (!ifs.good()) {
        return "";
    }
    std::string raftstate;
    ifs >> raftstate;
    ifs.close();
    return raftstate;
}

Persister::Persister(const int me)
    : m_raftStateFileName("raftstatePersist" + std::to_string(me) + ".txt"),
      m_snapshotFileName("snapshotPersist" + std::to_string(me) + ".txt"),
      m_raftStateSize(0) {
    bool fileOpenFlag = true;
    std::fstream file(m_raftStateFileName, std::ios::out);
    if (file.is_open()) {
        file.close();
    } else {
        fileOpenFlag = false;
    }
    file = std::fstream(m_snapshotFileName, std::ios::out);
    if (file.is_open()) {
        file.close();
    } else {
        fileOpenFlag = false;
    }
    if (!fileOpenFlag) {
        DPrintf("[func-Persister::Persister] file open error");
    }
    m_raftStateOutStream.open(m_raftStateFileName);
    m_snapshotOutStream.open(m_snapshotFileName);
}

Persister::~Persister() {
    if (m_raftStateOutStream.is_open()) {
        m_raftStateOutStream.close();
    }
    if (m_snapshotOutStream.is_open()) {
        m_snapshotOutStream.close();
    }
}

void Persister::clearRaftState() {
    m_raftStateSize = 0;
    if (m_raftStateOutStream.is_open()) {
        m_raftStateOutStream.close();
    }
    m_raftStateOutStream.open(m_raftStateFileName, std::ios::out | std::ios::trunc);
}

void Persister::clearSnapshot() {
    if (m_snapshotOutStream.is_open()) {
        m_snapshotOutStream.close();
    }
    m_snapshotOutStream.open(m_snapshotFileName, std::ios::out | std::ios::trunc);
}

void Persister::clearRaftStateAndSnapshot() {
    clearRaftState();
    clearSnapshot();
}