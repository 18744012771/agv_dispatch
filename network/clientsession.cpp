#include "clientsession.h"
#include "sessionmanager.h"
#include "../msgprocess.h"
#include "../common.h"

ClientSession::ClientSession(boost::asio::io_context &service):
    Session(service),
    json_len(0)
{
    timeout = 2*60*60;
}

void ClientSession::onStart()
{
    combined_logger->debug("new client session id {1} from:{0}",socket_.remote_endpoint().address().to_string(),sessionId);
    SessionManager::getInstance()->addClientSession(shared_from_base<ClientSession>());
}

void ClientSession::afterread()
{
    packageProcess();
}

void ClientSession::onStop()
{
    SessionManager::getInstance()->removeClientSession(shared_from_base<ClientSession>());
}

void ClientSession::packageProcess()
{
    if(buffer.size()<=json_len){
        return ;
    }

    while(true)
    {
        if(buffer.length()<=0)break;
        //寻找包头
        int pack_head = buffer.find(MSG_MSG_HEAD);
        if(pack_head>=0){
            //丢弃前面的无用数据
            buffer.removeFront(pack_head);
            //json长度
            json_len = buffer.getInt32(1);
            if(json_len==0){
                //空包,清除包头和长度信息。
                buffer.removeFront(1+sizeof(int32_t));
            }else if(json_len>0){
                if(json_len <= buffer.length()-1-sizeof(int32_t))
                {
                    Json::Reader reader(Json::Features::strictMode());
                    Json::Value root;
                    std::string sss = std::string(buffer.data(1+sizeof(int32_t)),json_len);
                    if (reader.parse(sss, root))
                    {
                        if (!root["type"].isNull() && !root["queuenumber"].isNull() && !root["todo"].isNull()) {
                            //combined_logger->info("RECV! session id={0}  len={1}  json=\n{2}" ,sessionId,json_len,sss);
                            MsgProcess::getInstance()->processOneMsg(root, shared_from_base<ClientSession>());
                        }
                    }
                    //清除报数据
                    buffer.removeFront(1+sizeof(int32_t)+json_len);
                    json_len = 0;
                }else{
                    //包不完整，继续接收
                    break;
                }
            }
        }else{
            //未找到包头
            buffer.clear();
            break;
        }
    }
}
