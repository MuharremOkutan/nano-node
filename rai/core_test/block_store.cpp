#include <gtest/gtest.h>
#include <rai/core/core.hpp>

#include <fstream>

TEST (block_store, construction)
{
    leveldb::Status init;
    rai::block_store db (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    auto now (db.now ());
    ASSERT_GT (now, 1408074640);
}

TEST (block_store, add_item)
{
    leveldb::Status init;
    rai::block_store db (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::send_block block;
    rai::uint256_union hash1 (block.hash ());
    auto latest1 (db.block_get (hash1));
    ASSERT_EQ (nullptr, latest1);
    ASSERT_FALSE (db.block_exists (hash1));
    db.block_put (hash1, block);
    auto latest2 (db.block_get (hash1));
    ASSERT_NE (nullptr, latest2);
    ASSERT_EQ (block, *latest2);
    ASSERT_TRUE (db.block_exists (hash1));
	db.block_del (hash1);
	auto latest3 (db.block_get (hash1));
	ASSERT_EQ (nullptr, latest3);
}

TEST (block_store, add_nonempty_block)
{
    leveldb::Status init;
    rai::block_store db (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::keypair key1;
    rai::send_block block;
    rai::uint256_union hash1 (block.hash ());
    rai::sign_message (key1.prv, key1.pub, hash1, block.signature);
    auto latest1 (db.block_get (hash1));
    ASSERT_EQ (nullptr, latest1);
    db.block_put (hash1, block);
    auto latest2 (db.block_get (hash1));
    ASSERT_NE (nullptr, latest2);
    ASSERT_EQ (block, *latest2);
}

TEST (block_store, add_two_items)
{
    leveldb::Status init;
    rai::block_store db (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::keypair key1;
    rai::send_block block;
    block.hashables.balance = 1;
    rai::uint256_union hash1 (block.hash ());
    rai::sign_message (key1.prv, key1.pub, hash1, block.signature);
    auto latest1 (db.block_get (hash1));
    ASSERT_EQ (nullptr, latest1);
    rai::send_block block2;
    block2.hashables.balance = 3;
    rai::uint256_union hash2 (block2.hash ());
    rai::sign_message (key1.prv, key1.pub, hash2, block2.signature);
    auto latest2 (db.block_get (hash2));
    ASSERT_EQ (nullptr, latest2);
    db.block_put (hash1, block);
    db.block_put (hash2, block2);
    auto latest3 (db.block_get (hash1));
    ASSERT_NE (nullptr, latest3);
    ASSERT_EQ (block, *latest3);
    auto latest4 (db.block_get (hash2));
    ASSERT_NE (nullptr, latest4);
    ASSERT_EQ (block2, *latest4);
    ASSERT_FALSE (*latest3 == *latest4);
}

TEST (block_store, add_receive)
{
    leveldb::Status init;
    rai::block_store db (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::keypair key1;
    rai::keypair key2;
    rai::receive_block block;
    rai::block_hash hash1 (block.hash ());
    auto latest1 (db.block_get (hash1));
    ASSERT_EQ (nullptr, latest1);
    db.block_put (hash1, block);
    auto latest2 (db.block_get (hash1));
    ASSERT_NE (nullptr, latest2);
    ASSERT_EQ (block, *latest2);
}

TEST (block_store, add_pending)
{
    leveldb::Status init;
    rai::block_store db (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::keypair key1;
    rai::block_hash hash1;
    rai::account sender1;
    rai::amount amount1;
    rai::account destination1;
    auto pending1 (db.pending_get (hash1, sender1, amount1, destination1));
    ASSERT_TRUE (pending1);
    db.pending_put (hash1, sender1, amount1, destination1);
    rai::account sender2;
    rai::amount amount2;
    rai::account destination2;
    auto pending2 (db.pending_get (hash1, sender2, amount2, destination2));
    ASSERT_EQ (sender1, sender2);
    ASSERT_EQ (amount1, amount2);
    ASSERT_EQ (destination1, destination2);
    ASSERT_FALSE (pending2);
    db.pending_del (hash1);
    auto pending3 (db.pending_get (hash1, sender2, amount2, destination2));
    ASSERT_TRUE (pending3);
}

TEST (block_store, add_genesis)
{
    leveldb::Status init;
    rai::block_store db (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::genesis genesis;
    genesis.initialize (db);
    rai::frontier frontier;
    ASSERT_FALSE (db.latest_get (rai::genesis_account, frontier));
    auto block1 (db.block_get (frontier.hash));
    ASSERT_NE (nullptr, block1);
    auto receive1 (dynamic_cast <rai::open_block *> (block1.get ()));
    ASSERT_NE (nullptr, receive1);
    ASSERT_LE (frontier.time, db.now ());
}

TEST (representation, changes)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::keypair key1;
    ASSERT_EQ (0, store.representation_get (key1.pub));
    store.representation_put (key1.pub, 1);
    ASSERT_EQ (1, store.representation_get (key1.pub));
    store.representation_put (key1.pub, 2);
    ASSERT_EQ (2, store.representation_get (key1.pub));
}

TEST (fork, adding_checking)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::keypair key1;
    rai::change_block block1 (key1.pub, rai::uint256_union (0), 0, rai::uint256_union (0), rai::uint256_union (0));
    ASSERT_EQ (nullptr, store.fork_get (block1.hash ()));
    rai::keypair key2;
    rai::change_block block2 (rai::uint256_union (0), rai::uint256_union (0), 0, rai::uint256_union (0), rai::uint256_union (0));
    store.fork_put (block1.hash (), block2);
    auto block3 (store.fork_get (block1.hash ()));
    ASSERT_EQ (block2, *block3);
}

TEST (bootstrap, simple)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::send_block block1;
    auto block2 (store.bootstrap_get (block1.previous ()));
    ASSERT_EQ (nullptr, block2);
    store.bootstrap_put (block1.previous (), block1);
    auto block3 (store.bootstrap_get (block1.previous ()));
    ASSERT_NE (nullptr, block3);
    ASSERT_EQ (block1, *block3);
    store.bootstrap_del (block1.previous ());
    auto block4 (store.bootstrap_get (block1.previous ()));
    ASSERT_EQ (nullptr, block4);
}

TEST (checksum, simple)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::block_hash hash0;
    ASSERT_TRUE (store.checksum_get (0x100, 0x10, hash0));
    rai::block_hash hash1;
    store.checksum_put (0x100, 0x10, hash1);
    rai::block_hash hash2;
    ASSERT_FALSE (store.checksum_get (0x100, 0x10, hash2));
    ASSERT_EQ (hash1, hash2);
    store.checksum_del (0x100, 0x10);
    rai::block_hash hash3;
    ASSERT_TRUE (store.checksum_get (0x100, 0x10, hash3));
}

TEST (block_store, empty_blocks)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    auto begin (store.blocks_begin ());
    auto end (store.blocks_end ());
    ASSERT_EQ (end, begin);
}

TEST (block_store, empty_accounts)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    auto begin (store.latest_begin ());
    auto end (store.latest_end ());
    ASSERT_EQ (end, begin);
}

TEST (block_store, one_block)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::send_block block1;
    store.block_put (block1.hash (), block1);
    auto begin (store.blocks_begin ());
    auto end (store.blocks_end ());
    ASSERT_NE (end, begin);
    auto hash1 (begin->first);
    ASSERT_EQ (block1.hash (), hash1);
    auto block2 (begin->second->clone ());
    ASSERT_EQ (block1, *block2);
    ++begin;
    ASSERT_EQ (end, begin);
}

TEST (block_store, empty_bootstrap)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    auto begin (store.bootstrap_begin ());
    auto end (store.bootstrap_end ());
    ASSERT_EQ (end, begin);
}

TEST (block_store, one_bootstrap)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::send_block block1;
    store.bootstrap_put (block1.hash (), block1);
    auto begin (store.bootstrap_begin ());
    auto end (store.bootstrap_end ());
    ASSERT_NE (end, begin);
    auto hash1 (begin->first);
    ASSERT_EQ (block1.hash (), hash1);
    auto block2 (begin->second->clone ());
    ASSERT_EQ (block1, *block2);
    ++begin;
    ASSERT_EQ (end, begin);
}

TEST (block_store, frontier_retrieval)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());;
    rai::account account1;
    rai::frontier frontier1;
    store.latest_put (account1, frontier1);
    rai::frontier frontier2;
    store.latest_get (account1, frontier2);
    ASSERT_EQ (frontier1, frontier2);
}

TEST (block_store, one_account)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::account account;
    rai::block_hash hash;
    store.latest_put (account, {hash, account, 42, 100});
    auto begin (store.latest_begin ());
    auto end (store.latest_end ());
    ASSERT_NE (end, begin);
    ASSERT_EQ (account, begin->first);
    ASSERT_EQ (hash, begin->second.hash);
    ASSERT_EQ (42, begin->second.balance.number ());
    ASSERT_EQ (100, begin->second.time);
    ++begin;
    ASSERT_EQ (end, begin);
}

TEST (block_store, two_block)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::send_block block1;
    block1.hashables.destination = 1;
    block1.hashables.balance = 2;
    std::vector <rai::block_hash> hashes;
    std::vector <rai::send_block> blocks;
    hashes.push_back (block1.hash ());
    blocks.push_back (block1);
    store.block_put (hashes [0], block1);
    rai::send_block block2;
    block2.hashables.destination = 3;
    block2.hashables.balance = 4;
    hashes.push_back (block2.hash ());
    blocks.push_back (block2);
    store.block_put (hashes [1], block2);
    auto begin (store.blocks_begin ());
    auto end (store.blocks_end ());
    ASSERT_NE (end, begin);
    auto hash1 (begin->first);
    ASSERT_NE (hashes.end (), std::find (hashes.begin (), hashes.end (), hash1));
    auto block3 (begin->second->clone ());
    ASSERT_NE (blocks.end (), std::find (blocks.begin (), blocks.end (), *block3));
    ++begin;
    ASSERT_NE (end, begin);
    auto hash2 (begin->first);
    ASSERT_NE (hashes.end (), std::find (hashes.begin (), hashes.end (), hash2));
    auto block4 (begin->second->clone ());
    ASSERT_NE (blocks.end (), std::find (blocks.begin (), blocks.end (), *block4));
    ++begin;
    ASSERT_EQ (end, begin);
}

TEST (block_store, two_account)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::account account1 (1);
    rai::block_hash hash1 (2);
    rai::account account2 (3);
    rai::block_hash hash2 (4);
    store.latest_put (account1, {hash1, account1, 42, 100});
    store.latest_put (account2, {hash2, account2, 84, 200});
    auto begin (store.latest_begin ());
    auto end (store.latest_end ());
    ASSERT_NE (end, begin);
    ASSERT_EQ (account1, begin->first);
    ASSERT_EQ (hash1, begin->second.hash);
    ASSERT_EQ (42, begin->second.balance.number ());
    ASSERT_EQ (100, begin->second.time);
    ++begin;
    ASSERT_NE (end, begin);
    ASSERT_EQ (account2, begin->first);
    ASSERT_EQ (hash2, begin->second.hash);
    ASSERT_EQ (84, begin->second.balance.number ());
    ASSERT_EQ (200, begin->second.time);
    ++begin;
    ASSERT_EQ (end, begin);
}

TEST (block_store, latest_find)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::account account1 (1);
    rai::block_hash hash1 (2);
    rai::account account2 (3);
    rai::block_hash hash2 (4);
    store.latest_put (account1, {hash1, account1, 100});
    store.latest_put (account2, {hash2, account2, 200});
    auto first (store.latest_begin ());
    auto second (store.latest_begin ());
    ++second;
    auto find1 (store.latest_begin (1));
    ASSERT_EQ (first, find1);
    auto find2 (store.latest_begin (3));
    ASSERT_EQ (second, find2);
    auto find3 (store.latest_begin (2));
    ASSERT_EQ (second, find3);
}

TEST (block_store, bad_path)
{
    leveldb::Status init;
    rai::block_store store (init, boost::filesystem::path {});
    ASSERT_FALSE (init.ok ());
}

TEST (block_store, already_open)
{
    auto path (boost::filesystem::unique_path ());
    boost::filesystem::create_directories (path);
    std::ofstream file;
    file.open ((path / "accounts.ldb").string ().c_str ());
    ASSERT_TRUE (file.is_open ());
    leveldb::Status init;
    rai::block_store store (init, path);
    ASSERT_FALSE (init.ok ());
}

TEST (block_store, delete_iterator_entry)
{
    leveldb::Status init;
    rai::block_store store (init, rai::block_store_temp);
    ASSERT_TRUE (init.ok ());
    rai::send_block block1;
    block1.hashables.previous = 1;
    store.block_put (block1.hash (), block1);
    rai::send_block block2;
    block2.hashables.previous = 2;
    store.block_put (block2.hash (), block2);
    auto current (store.blocks_begin ());
    ASSERT_NE (store.blocks_end (), current);
    store.block_del (current->first);
    ++current;
    ASSERT_NE (store.blocks_end (), current);
    store.block_del (current->first);
    ++current;
    ASSERT_EQ (store.blocks_end (), current);
}