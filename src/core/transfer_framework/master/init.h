//
//  master/init.h
//  SwiftSnails
//
//  Created by Chunwei on 3/17/15.
//  Copyright (c) 2015 Chunwei. All rights reserved.
//
#ifndef SwiftSnails_core_transfer_framework_master_init_h_
#define SwiftSnails_core_transfer_framework_master_init_h_
#include "../../../utils/Barrier.h"
#include "../../../utils/Timer.h"
#include "../message_classes.h"
#include "../ServerWorkerRoute.h"
#include "../../transfer/transfer.h"
namespace swift_snails {

/*
 * when timeout or all the nodes has been registered
 */
typedef std::function<void()> void_lamb;


class MasterTransferInit {
	typedef Transfer<ServerWorkerRoute> transfer_t;
public:
    explicit MasterTransferInit() {
        LOG(INFO) << "init Master ...";
        // register master's keys to config
        global_config().register_config("expected_node_num");
        global_config().register_config("master_time_out");
        // TODO should read from config file
        expected_node_num = global_config().get_config("expected_node_num").to_int32();     // nodes include server and worker
        _timer.set_time_span(
            global_config().get_config("master_time_out").to_int32() ); 
        _timer.start();
    }

    void operator() () {
        register_init_message_class();
        CHECK(!gtransfer.async_channel()->closed()) << "channel should not been closed before transfer is deconstructed";
        wait_for_workers_register_route();
    }

    void register_init_message_class() {
        LOG(WARNING) << "register message class ...";
    	auto handler = node_init_address;
        gtransfer.message_class().add(NODE_INIT_ADDRESS, std::move(handler));
        LOG(WARNING) << "end register message class";
    }

    void wait_for_workers_register_route() {
        std::function<bool()> cond_func = [this]() {
            return ( _timer.timeout() 
                || registered_node_num == expected_node_num);
        };

        void_lamb set_flag = [] { };
        // set a thread to try unblock current thread 
        // when timeout 
        void_lamb timeout_try_unblock = [this] {
            LOG(WARNING) << "try unblock is called ...";
            LOG(WARNING) << "wait for 2 s";
            std::this_thread::sleep_for( std::chrono::milliseconds(1000 + 1000 * _timer.time_span() ));
            void_lamb handle = []{};
            _wait_init_barrier.unblock(handle);
        };
        gtransfer.async_channel()->push(std::move(timeout_try_unblock));
        _wait_init_barrier.block(set_flag, cond_func); 
    }

protected:
    // message-class handlers   ---------------------------------
    // node register address to master

    transfer_t::msgcls_handler_t node_init_address = [this](std::shared_ptr<Request> request, Request& response) {
        LOG(INFO) << "get node register";
        IP ip;
        request->cont >> ip;
        std::string addr = "tcp://" + ip.to_string();
        auto& gtransfer = global_transfer<ServerWorkerRoute>();
        // tell server/worker by clent_id
        // -1 or 0
        CHECK(request->meta.client_id <= 0);
        int id = gtransfer.route().register_node_( request->meta.client_id == 0, std::move(addr));

        registered_node_num ++;

        CHECK(id >= 0);
        // check status 
        // and notify the main thread to check the cond_variable 
        if(registered_node_num >= expected_node_num - 2) {
            void_lamb handler = []{};
            _wait_init_barrier.unblock(handler);
        }
    };

private:
    Transfer<ServerWorkerRoute>& gtransfer = global_transfer<ServerWorkerRoute>();
    int registered_node_num = 0;
    // TODO should read from config file
    int expected_node_num = 10; // TODO read from config file

    Timer _timer;
    CompBarrier _wait_init_barrier;

};


}; // end namespace swift_snails
#endif