#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/types.h>
#include <eosiolib/crypto.h>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

class verifier : public contract {
  public:
      using contract::contract;

      [[eosio::action]]
      void addutxo(name relayer,
                   const string pkeyFrom,
                   const string pkeyTo,
                   const string pkeyFee,
                   asset amount,
                   asset fee,
                   string memo) {
        require_auth(relayer);
        eosio_assert(pkeyFrom != pkeyTo, "cannot transfer to self");

        capi_checksum256 digest;
        const char* data = "test_data";
        sha256(data, sizeof(data), &digest);

        const string signature = "SIG_K1_KfQ57wLFFiPR85zjuQyZsn7hK3jRicHXg4qETxLvxHQTHHejveGtiYBSx6Z68xBZYrY9Fihz74makocnSLQFBwaHTg6Aaa";
        eosio_assert(recover_key(digest, signature, 101, pkeyFrom, 53), "digest and signature do not match");

        auto sym = amount.symbol.name();
        stats statstable(_self, sym);
        const auto& st = statstable.get(sym);

        eosio_assert(amount.is_valid(), "invalid quantity" );
        eosio_assert(fee.is_valid(), "invalid quantity" );
        eosio_assert(amount.amount > 0, "must transfer positive quantity");
        eosio_assert(fee.amount >= 0, "must transfer non negative quantity");
        eosio_assert(amount.symbol == st.supply.symbol, "symbol precision mismatch");
        eosio_assert(fee.symbol == st.supply.symbol, "symbol precision mismatch");
        eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

        // once for the amount from to to
        sub_balance(pkeyFrom, amount);
        add_balance(pkeyTo, amount, pkeyFrom);

        // second time to give the relayer the fee
      }

  private:
    bool recover_key(const capi_checksum256 digest,
                     const string signature,
                     size_t siglen,
                     const string pub,
                     size_t publen) {
      return false;
    }

    void sub_balance(const string owner, asset value) {
      txtable transactions(_self, _self.value);

      const auto& from = from_acnts.get(value.symbol.name(), "no balance object found");
      eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

      if (from.balance.amount == value.amount) {
        from_acnts.erase( from );
      } else {
        from_acnts.modify( from, owner, [&]( auto& a ) {
            a.balance -= value;
        });
      }
    }

    void add_balance(const string owner, asset value, const string ram_payer) {
      txtable transactions(_self, _self.value);

      auto to = to_acnts.find( value.symbol.name() );
      if (to == to_acnts.end()) {
        to_acnts.emplace( ram_payer, [&]( auto& a ){
          a.balance = value;
        });
      } else {
        to_acnts.modify( to, 0, [&]( auto& a ) {
          a.balance += value;
        });
      }
    }

    struct account {
      asset balance;

      uint64_t primary_key() const { return balance.symbol.name(); }
    };

    struct [[eosio::table]] currstats {
      asset supply;
      asset max_supply;
      string issuer;

      uint64_t primary_key() const { return supply.symbol.name(); }
    };

    typedef eosio::multi_index<"accounts"_n, account> accounts;
    typedef eosio::multi_index<"stats"_n, currstats> stats;

};
EOSIO_DISPATCH(verifier, (addutxo))