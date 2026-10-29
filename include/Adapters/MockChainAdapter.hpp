#pragma once
#include <unordered_map>
#include "IBlockchainAdapter.hpp"

using namespace std;

// Adapter in-memory siêu nhẹ: không nonce, không gas. Chấp nhận MỌI loại giao dịch
// bằng cách chỉ dùng giao diện chung ITransaction (sender/receiver/amount).
class MockChainAdapter : public IBlockchainAdapter {
private:
    string chainId_;
    unordered_map<string, double>             balances_;
    unordered_map<string, SmartContractState> contracts_;
    vector<string>                            history_;   // nhật ký tx đã chạy
    double lockedPool_ = 0.0;

public:
    explicit MockChainAdapter(string chainId = "MockChain") : chainId_(std::move(chainId)) {}

    void setBalance(const string& addr, double amount) { balances_[addr] = amount; }
    const vector<string>& getHistory() const { return history_; }
    double getLockedPool() const { return lockedPool_; }

    string getChainId() const override { return chainId_; }

    double getBalance(const string& address) override {
        auto it = balances_.find(address);
        return it == balances_.end() ? 0.0 : it->second;
    }

    TransactionResult sendTransaction(const ITransaction& tx) override {
        if (tx.getAmount() <= 0)
            return TransactionResult{false, "", "So luong phai > 0", 0.0};
        if (getBalance(tx.getSender()) < tx.getAmount())
            return TransactionResult{false, "", "Insufficient funds", 0.0};

        balances_[tx.getSender()]   -= tx.getAmount();
        balances_[tx.getReceiver()] += tx.getAmount();
        history_.push_back(tx.getTxHash());
        return TransactionResult{true, tx.getTxHash(), "", 0.0};
    }

    bool lockAsset(const string& sender, double amount) override {
        if (amount <= 0 || getBalance(sender) < amount) return false;
        balances_[sender] -= amount;
        lockedPool_       += amount;
        return true;
    }

    bool mintOrReleaseAsset(const string& receiver, double amount) override {
        if (amount <= 0) return false;
        balances_[receiver] += amount;
        return true;
    }

    SmartContractState getContractState(const string& addr) override {
        auto it = contracts_.find(addr);
        if (it != contracts_.end()) return it->second;
        SmartContractState empty; empty.contractAddress = addr;
        return empty;
    }
};
