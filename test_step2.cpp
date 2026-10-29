#include <iostream>
#include <cmath>
#include <vector>
#include "include/Common/Types.hpp"
#include "include/Common/Transaction.hpp"
#include "include/Factory/TransactionFactory.hpp"
#include "include/Adapters/IBlockchainAdapter.hpp"
#include "include/Adapters/EVMAdapter.hpp"
#include "include/Adapters/MockChainAdapter.hpp"

using namespace std;

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, name) do { if (cond) { ++g_pass; cout << "  [PASS] " << name << "\n"; } \
                               else { ++g_fail; cout << "  [FAIL] " << name << "\n"; } } while (0)
static bool approx(double a, double b) { return fabs(a - b) < 1e-9; }

int main() {
    cout << "=== UNIT TEST BUOC 2: FACTORY + ADAPTER ===\n";

    // ---------------- Factory ----------------
    cout << "\n[1] Transaction Factory\n";
    TransactionPayload p;
    p.sender = "Alice"; p.receiver = "Bob"; p.amount = 25.5; p.nonce = 0;

    auto evmTx  = TransactionFactoryProvider::create(ChainType::EVM,  p);
    auto utxoTx = TransactionFactoryProvider::create(ChainType::UTXO, p);
    cout << "  " << evmTx->describe()  << "\n";
    cout << "  " << utxoTx->describe() << "\n";

    auto* e = dynamic_cast<EVMTransaction*>(evmTx.get());
    auto* u = dynamic_cast<UTXOTransaction*>(utxoTx.get());
    CHECK(e != nullptr && evmTx->getType() == ChainType::EVM,  "Factory EVM tra ve EVMTransaction");
    CHECK(u != nullptr && utxoTx->getType() == ChainType::UTXO, "Factory UTXO tra ve UTXOTransaction");
    CHECK(e && e->gasLimit == 21000 && e->gasPrice > 0,         "EVM Tx co gasPrice/gasLimit hop le");
    CHECK(u && u->inputs.size() == 1 && u->outputs.size() == 1 && u->signatures.size() == 1,
          "UTXO Tx co 1 input, 1 output, 1 signature");
    CHECK(u && approx(u->totalIn(), u->totalOut() + u->fee()) && u->fee() > 0,
          "UTXO: tong input = tong output + fee");

    // ---------------- EVMAdapter ----------------
    cout << "\n[2] EVMAdapter\n";
    EVMAdapter eth("Ethereum");
    eth.setBalance("Alice", 100.0);

    auto r1 = eth.sendTransaction(*evmTx);
    double fee = 21000 * 20.0 * 1e-9;
    CHECK(r1.isSuccess, "Gui Tx EVM hop le -> thanh cong");
    CHECK(approx(eth.getBalance("Bob"), 25.5), "Bob nhan dung 25.5");
    CHECK(approx(eth.getBalance("Alice"), 100.0 - 25.5 - fee), "Alice bi tru value + phi gas");
    CHECK(eth.getNonce("Alice") == 1, "Nonce cua Alice tang len 1");
    CHECK(r1.gasUsed == 21000, "gasUsed = 21000");

    auto r2 = eth.sendTransaction(*evmTx);   // gửi lại cùng tx (nonce=0 cũ)
    CHECK(!r2.isSuccess && r2.errorMessage.find("nonce") != string::npos, "Tx lap lai bi tu choi (Invalid nonce)");

    TransactionPayload big = p; big.amount = 1000; big.nonce = eth.getNonce("Alice");
    auto r3 = eth.sendTransaction(*TransactionFactoryProvider::create(ChainType::EVM, big));
    CHECK(!r3.isSuccess && r3.errorMessage == "Insufficient funds", "Khong du so du -> Insufficient funds");

    auto r4 = eth.sendTransaction(*utxoTx);
    CHECK(!r4.isSuccess, "EVMAdapter tu choi giao dich UTXO");

    // ---------------- Smart contract state ----------------
    cout << "\n[3] Contract state qua Adapter\n";
    eth.deployContract("0xBridge");
    TransactionPayload call; call.sender = "Alice"; call.receiver = "0xBridge";
    call.amount = 1.0; call.data = "lock(25.5)"; call.nonce = eth.getNonce("Alice");
    auto r5 = eth.sendTransaction(*TransactionFactoryProvider::create(ChainType::EVM, call));
    auto st = eth.getContractState("0xBridge");
    CHECK(r5.isSuccess, "Goi hop dong 0xBridge thanh cong");
    CHECK(st.storage["call_count"] == "1" && st.storage["last_calldata"] == "lock(25.5)",
          "State hop dong cap nhat: call_count=1, last_calldata=lock(25.5)");
    CHECK(r5.gasUsed > 21000, "Goi hop dong ton nhieu gas hon chuyen coin thuong");

    // ---------------- MockChainAdapter ----------------
    cout << "\n[4] MockChainAdapter\n";
    MockChainAdapter sol("Solana");
    sol.setBalance("Alice", 50.0);
    auto r6 = sol.sendTransaction(*utxoTx);   // Mock chấp nhận cả giao dịch UTXO
    CHECK(r6.isSuccess && approx(sol.getBalance("Bob"), 25.5) && approx(sol.getBalance("Alice"), 24.5),
          "Mock xu ly Tx UTXO: Alice 50 -> 24.5, Bob 0 -> 25.5");
    sol.setBalance("Alice", 100.0);           // nạp thêm để đủ trả giao dịch kế tiếp
    auto r7 = sol.sendTransaction(*evmTx);    // và cả EVM
    CHECK(r7.isSuccess && sol.getHistory().size() == 2, "Mock cung xu ly Tx EVM, nhat ky co 2 giao dich");

    // ---------------- Đa hình qua IBlockchainAdapter ----------------
    cout << "\n[5] Da hinh: cung 1 doan code cho moi Adapter\n";
    EVMAdapter e2("BNB Chain"); e2.setBalance("Carol", 10);
    MockChainAdapter m2("Near"); m2.setBalance("Carol", 10);
    vector<IBlockchainAdapter*> chains = { &e2, &m2 };
    bool allOk = true;
    for (auto* c : chains) {
        TransactionPayload pp; pp.sender = "Carol"; pp.receiver = "Dave"; pp.amount = 2.0;
        auto tx = TransactionFactoryProvider::create(ChainType::EVM, pp);
        auto r = c->sendTransaction(*tx);
        cout << "  " << c->getChainId() << ": ok=" << r.isSuccess
             << ", Dave=" << c->getBalance("Dave") << "\n";
        allOk = allOk && r.isSuccess && approx(c->getBalance("Dave"), 2.0);
    }
    CHECK(allOk, "Ca 2 Adapter xu ly duoc qua con tro IBlockchainAdapter*");

    // ---------------- Lock & Mint xuyên chuỗi ----------------
    cout << "\n[6] Lock (Ethereum) -> Mint (Solana)\n";
    EVMAdapter chainA("Ethereum"); chainA.setBalance("Alice", 100);
    MockChainAdapter chainB("Solana");
    CrossChainMessage msg; msg.sender = "Alice"; msg.receiver = "Bob"; msg.amount = 25.5;
    bool locked = chainA.lockAsset(msg.sender, msg.amount);
    bool minted = locked && chainB.mintOrReleaseAsset(msg.receiver, msg.amount);
    CHECK(locked && minted, "Lock o chuoi A, Mint o chuoi B thanh cong");
    CHECK(approx(chainA.getBalance("Alice"), 74.5) && approx(chainA.getLockedPool(), 25.5)
          && approx(chainB.getBalance("Bob"), 25.5), "So du: Alice 74.5, Pool 25.5, Bob(Solana) 25.5");
    CHECK(!chainA.lockAsset("Alice", 9999), "Lock vuot so du bi tu choi");

    cout << "\n=======================================\n";
    cout << "KET QUA: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    cout << "=======================================\n";
    return g_fail == 0 ? 0 : 1;
}
