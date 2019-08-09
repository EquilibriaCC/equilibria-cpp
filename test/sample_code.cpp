#include <stdio.h>
#include <iostream>
#include "wallet2.h"
#include "wallet/monero_wallet.h"

using namespace std;

/**
 * This code introduces the API.
 *
 * NOTE: depending on feedback, fields might become private and accessible only
 * through public acessors/mutators for pure object-oriented.
 */
int main(int argc, const char* argv[]) {

  // create a wallet from a mnemonic phrase
  string mnemonic = "hefty value later extra artistic firm radar yodel talent future fungal nutshell because sanity awesome nail unjustly rage unafraid cedar delayed thumbs comb custom sanity";
  monero_wallet* wallet_restored = monero_wallet::create_wallet_from_mnemonic(
      "MyWalletRestored",                               // wallet path and name
      "supersecretpassword123",                         // wallet password
      monero_network_type::STAGENET,                    // network type
      mnemonic,                                         // mnemonic phrase
      monero_rpc_connection("http://localhost:38081"),  // daemon connection
      380104                                            // restore height
  );

  // sync the wallet
  wallet_restored->sync();          // one time synchronous sync
  wallet_restored->start_syncing();  // continuously sync as an asynchronous thread in the background

  // get balance, account, subaddresses
  string primary_address = wallet_restored->get_primary_address();
  uint64_t balance = wallet_restored->get_balance();    // can specify account and subaddress indices
  monero_account account = wallet_restored->get_account(1, true);     // get account with subaddresses
  uint64_t unlocked_account_balance = account.m_unlocked_balance.get(); // get boost::optional value

  // query a transaction by id
  monero_tx_query tx_query;
  tx_query.m_id = "3276252c5a545b90c8e147fcde45d3e1917726470a8f7d4c8977b527a44dfd15";
  shared_ptr<monero_tx_wallet> tx = wallet_restored->get_txs(tx_query)[0];
  for (const shared_ptr<monero_incoming_transfer> in_transfer : tx->m_incoming_transfers) {
    uint64_t in_amount = in_transfer->m_amount.get();
    int account_index = in_transfer->m_account_index.get();
  }

  // query incoming transfers to account 1
  monero_transfer_query transfer_query;
  transfer_query.m_is_incoming = true;
  transfer_query.m_account_index = 1;
  vector<shared_ptr<monero_transfer>> transfers = wallet_restored->get_transfers(transfer_query);

  // query unspent outputs
  monero_output_query output_query;
  output_query.m_is_spent = false;
  vector<shared_ptr<monero_output_wallet>> outputs = wallet_restored->get_outputs(output_query);

  // create a new wallet with a random mnemonic phrase
  monero_wallet* wallet_random = monero_wallet::create_wallet_random(
      "MyWalletRandom",                                 // wallet path and name
      "supersecretpassword123",                         // wallet password
      monero_network_type::STAGENET,                    // network type
      monero_rpc_connection("http://localhost:38081"),  // daemon connection
      "English"
  );

  // continuously synchronize the wallet
  wallet_random->start_syncing();

  // get wallet info
  string random_mnemonic = wallet_random->get_mnemonic();
  string address = wallet_random->get_primary_address();
  uint64_t height = wallet_random->get_height();
  bool is_synced = wallet_random->is_synced();

  // be notified when the wallet receives funds
  struct : monero_wallet_listener {
    void on_output_received(const monero_output_wallet& output) {
      cout << "Wallet received funds!" << endl;
      int account_index = output.m_account_index.get();
      int subaddress_index = output.m_subaddress_index.get();
      shared_ptr<monero_key_image> key_image = output.m_key_image.get();
    }
  } my_listener;
  wallet_random->set_listener(my_listener);

  // send funds from the restored wallet to the random wallet
  shared_ptr<monero_tx_wallet> sent_tx = wallet_restored->send(0, wallet_random->get_address(1, 0), 50000);
  bool in_pool = sent_tx->m_in_tx_pool.get();  // true

  // create a request to send funds to multiple destinations in the random wallet
  monero_send_request send_request = monero_send_request();
  send_request.m_account_index = 1;                // withdraw funds from this account
  send_request.m_subaddress_indices = vector<uint32_t>();
  send_request.m_subaddress_indices.push_back(0);
  send_request.m_subaddress_indices.push_back(1);  // withdraw funds from these subaddresses within the account
  send_request.m_priority = monero_send_priority::UNIMPORTANT;  // no rush
  vector<shared_ptr<monero_destination>> destinations;  // specify the recipients and their amounts
//  destinations.push_back(make_shared<monero_destination>(wallet_random->get_address(1, 0), 50000)); // TODO: destination constructor
//  destinations.push_back(make_shared<monero_destination>(wallet_random->get_address(2, 0), 50000));
  send_request.m_destinations = destinations;

  // create the transaction, confirm with the user, and relay to the network
  shared_ptr<monero_tx_wallet> created_tx = wallet_restored->create_tx(send_request);
  uint64_t fee = created_tx->m_fee.get();   // "Are you sure you want to send ...?"
  wallet_restored->relay_tx(*created_tx);  // submit the transaction to the Monero network which will notify the recipient wallet
}