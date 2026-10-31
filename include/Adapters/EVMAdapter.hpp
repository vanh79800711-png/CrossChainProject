#pragma once
#include <unordered_map>
#include "IBlockchainAdapter.hpp"

using namespace std;

// Adapter mô phỏng mạng EVM (Ethereum/BNB): mô hình ACCOUNT, có nonce + gas + hợp đồng.
class EVMAdapter : public IBlockchainAdapter {
private:
    string chainId_;
    unordered_map<string, double>              balances_;
    unordered_map<string, uint64_t>            nonces_;
    unordered_map<string, SmartContractState>  contracts_;
    double lockedPool_ = 0.0;

    static constexpr uint64_t BASE_GAS          = 21000;
    static constexpr uint64_t GAS_PER_DATA_BYTE = 16;
    static constexpr uint64_t CONTRACT_CALL_GAS = 5000;
    static constexpr double   GWEI              = 1e-9;

    static TransactionResult fail(const string& msg) { return TransactionResult{false, "", msg, 0.0}; }

public:
    explicit EVMAdapter(string chainId = "Ethereum") : chainId_(std::move(chainId)) {}

    // ---- Tiện ích cho môi trường test (faucet, deploy) ----
    void setBalance(const string& addr, double amount) { balances_[addr] = amount; }
    uint64_t getNonce(const string& addr) const {
        auto it = nonces_.find(addr);
        return it == nonces_.end() ? 0 : it->second;
    }
    void deployContract(const string& addr) { contracts_[addr].contractAddress = addr; }
    double getLockedPool() const { return lockedPool_; }

    // ---- IBlockchainAdapter ----
    string getChainId() const override { return chainId_; }

    double getBalance(const string& address) override {
        auto it = balances_.find(address);
        return it == balances_.end() ? 0.0 : it->second;
    }

    TransactionResult sendTransaction(const ITransaction& tx) override {
        // 1. Adapter chỉ hiểu giao dịch EVM
        const EVMTransaction* evm = dynamic_cast<const EVMTransaction*>(&tx);
        if (!evm) return fail("EVMAdapter chi chap nhan giao dich EVM");
        if (evm->value < 0)  return fail("Gia tri chuyen khong hop le");

        // 2. Kiểm tra nonce (chống replay / sai thứ tự)
        if (evm->nonce != getNonce(evm->from))
            return fail("Invalid nonce: can " + to_string(getNonce(evm->from)) +
                        ", nhan " + to_string(evm->nonce));

        // 3. Tính gas giả lập
        bool isContractCall = contracts_.count(evm->to) > 0;
        uint64_t gasUsed = BASE_GAS + GAS_PER_DATA_BYTE * evm->data.size()
                         + (isContractCall ? CONTRACT_CALL_GAS : 0);
        if (gasUsed > evm->gasLimit) return fail("Out of gas");

        // 4. Kiểm tra số dư: value + phí gas
        double fee       = gasUsed * evm->gasPrice * GWEI;
        double totalCost = evm->value + fee;
        if (getBalance(evm->from) + 1e-12 < totalCost) return fail("Insufficient funds");

        // 5. Thực thi: cập nhật state
        balances_[evm->from] -= totalCost;
        balances_[evm->to]   += evm->value;
        nonces_[evm->from]++;

        if (isContractCall) {
            auto& st = contracts_[evm->to];
            int calls = st.storage.count("call_count") ? stoi(st.storage["call_count"]) : 0;
            st.storage["call_count"]    = to_string(calls + 1);
            st.storage["last_calldata"] = evm->data;
        }
        return TransactionResult{true, evm->getTxHash(), "", (double)gasUsed};
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
