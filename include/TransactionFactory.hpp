#pragma once
#include <memory>
#include <stdexcept>
#include "../Common/Transaction.hpp"

using namespace std;

// ======================= Abstract Factory =======================
class ITransactionFactory {
public:
    virtual ~ITransactionFactory() = default;
    virtual unique_ptr<ITransaction> createTransaction(const TransactionPayload& payload) const = 0;
    virtual ChainType supportedChain() const = 0;
};

// ======================= EVM Factory =======================
class EVMTransactionFactory : public ITransactionFactory {
private:
    double defaultGasPrice_;   // Gwei
public:
    explicit EVMTransactionFactory(double gasPrice = 20.0) : defaultGasPrice_(gasPrice) {}

    ChainType supportedChain() const override { return ChainType::EVM; }

    unique_ptr<ITransaction> createTransaction(const TransactionPayload& p) const override {
        // Chuyển coin thuần: 21000 gas. Có calldata (gọi hợp đồng): đặt hạn mức 100000.
        uint64_t gasLimit = p.data.empty() ? 21000 : 100000;
        return make_unique<EVMTransaction>(p.nonce, defaultGasPrice_, gasLimit,
                                           p.sender, p.receiver, p.amount, p.data);
    }
};

// ======================= UTXO Factory =======================
class UtxoTransactionFactory : public ITransactionFactory {
private:
    double fee_;
public:
    explicit UtxoTransactionFactory(double fee = 0.0001) : fee_(fee) {}

    ChainType supportedChain() const override { return ChainType::UTXO; }

    unique_ptr<ITransaction> createTransaction(const TransactionPayload& p) const override {
        auto tx = make_unique<UTXOTransaction>();
        // Giả lập bước "chọn UTXO": 1 input vừa đủ trả (amount + fee), không có tiền thối.
        tx->inputs.push_back({ makePseudoHash("utxo:" + p.sender), 0, p.sender, p.amount + fee_ });
        tx->outputs.push_back({ p.receiver, p.amount });
        tx->signatures.push_back("SIG(" + p.sender + ")");   // chữ ký giả định
        return tx;
    }
};

// ======================= Provider (chọn Factory theo ChainType) =======================
class TransactionFactoryProvider {
public:
    static unique_ptr<ITransactionFactory> getFactory(ChainType type) {
        switch (type) {
            case ChainType::EVM:  return make_unique<EVMTransactionFactory>();
            case ChainType::UTXO: return make_unique<UtxoTransactionFactory>();
        }
        throw invalid_argument("ChainType khong duoc ho tro");
    }

    // Tiện ích: tạo thẳng giao dịch theo loại mạng đích
    static unique_ptr<ITransaction> create(ChainType type, const TransactionPayload& payload) {
        return getFactory(type)->createTransaction(payload);
    }
};
