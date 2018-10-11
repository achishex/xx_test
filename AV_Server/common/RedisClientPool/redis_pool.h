/**
 * @file: redis_pool.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00001
 * @date: 2018-07-29
 */

#ifndef _REDIS_POOL_H_ 
#define _REDIS_POOL_H_

#include "redis_conn_pool.h"
#include <condition_variable>
#include <mutex>
#include <map>
#include <vector>


class FS_RedisConnPool: public RedisConnPool 
{
 public:
  FS_RedisConnPool(int32_t _max_conn_nums, int32_t _max_idle_tm, 
                   const std::string& _host,
                   unsigned _port, unsigned _db_index);
  virtual ~FS_RedisConnPool();
  virtual redisContext* Grub();
  void ReleaseConn(const redisContext* _rc, bool _is_bad);
  //
 protected:
  virtual redisContext* CreateRedis();
  virtual void ReleaseConn(const redisContext* _rc);
  virtual void DestroyRedis(redisContext* _rc);
  virtual unsigned GetMaxIdleTime();
 private:
  int32_t conn_in_use_nums_;
  int32_t conn_max_nums_;
  int32_t redis_idle_max_tm_;
  //
  std::string str_host_;
  unsigned ui_port_;
  unsigned database_index_;

  std::mutex conn_in_use_mutex_;
  std::condition_variable conn_in_use_mutx_cond_;
};

/////////////////////////////////////////////////
enum ReplyType
{
  STRING    = 1,
  ARRAY     = 2,
  INTEGER   = 3,
  NIL       = 4,
  STATUS    = 5,
  ERROR     = 6
};

class Reply
{
 public:
  ReplyType Type() const {return type_;}
  long long Integer() const {return integer_;}
  const std::string& Str() const {return str_;}
  const std::vector<Reply>& Elements() const {return elements_;}

  Reply(redisReply *reply = NULL):type_(ERROR), integer_(0)
  {
    if (reply == NULL)
      return;

    type_ = static_cast<ReplyType>(reply->type);
    switch(type_) {
      case ERROR:
      case STRING:
      case STATUS:
        str_ = std::string(reply->str, reply->len);
        break;
      case INTEGER:
        integer_ = reply->integer;
        break;
      case ARRAY:
        for (size_t i = 0; i < reply->elements; ++i){
          elements_.push_back(Reply(reply->element[i]));
        }
        break;
      default:
        break;
    }
  }

  ~Reply(){}

  void Print() const {
    if (Type() == NIL) {
      printf("NIL.\n");
    }
    if (Type() == STRING) {
      printf("STRING:%s\n", Str().c_str());
    }
    if (Type() == ERROR) {
      printf("ERROR:%s\n", Str().c_str());
    }
    if (Type() == STATUS) {
      printf("STATUS:%s\n", Str().c_str());
    }
    if (Type() == INTEGER) {
      printf("INTEGER:%lld\n", Integer());
    }
    if (Type() == ARRAY) {
      const std::vector<Reply>& elements = Elements();

      for (size_t j = 0; j != elements.size(); j++) 
      {
        printf("%lu) ", j);
        elements[j].Print();
      }
    }
  }

 private:
  ReplyType           type_;
  std::string         str_;
  long long           integer_;
  std::vector<Reply>  elements_;
};

//
class FS_RedisInterface: public FS_RedisConnPool 
{
 public:
  FS_RedisInterface(int32_t _max_conn_nums, int32_t _max_idle_tm, 
                    const std::string& _host,
                    unsigned _port, unsigned _db_index);
  virtual ~FS_RedisInterface() {}
  //key interface
  bool Ping(std::string& _ret);
  bool Set(const std::string& sKey, const std::string& sVal);
  bool Get(const std::string& sKey, std::string& sVal);

  bool Del(const std::string& _key);
  bool ExpireTm(const std::string& _key, const unsigned _exp_tm);
  //hash iterface.
  bool HSet(const std::string& _key, const std::string& _field, 
            const std::string& _val);
  bool HGet(const std::string& _key, 
            const std::string& _field, std::string* _val);
  bool HMSet(const std::string& _key, 
             const std::map<std::string, std::string>& _field_val);
  bool HMGet(const std::string& _key,
             std::map<std::string, std::string>& _val);
  bool HGetAll(const std::string& _key,
               std::map<std::string, std::string>& _val);
  bool HVals(const std::string& _key, std::vector<std::string>& _val);
  bool HDel(const std::string& _key, const std::vector<std::string>& _fields);
  //icount == 0, not use count filed
  bool Scan(int& iCursor, std::vector<std::string>& vList, const std::string& sPatten = "", int iCount = 0);
 private:
  bool FreeReplyObj(redisReply* _preply);
  bool CheckRetInt(redisReply* _preply,
                   redisContext* _rc, const std::string& _cmd);
  bool CheckRetString(redisReply* _preply,
                      redisContext* _rc, const std::string& _cmd);
  bool CheckRetNil(redisReply* _preply, 
                   redisContext* _rc, const std::string& _cmd);
  bool CheckRetArr(redisReply* _preply, 
                   redisContext* _rc, const std::string& _cmd);
  bool CheckReplySucc(redisReply* _preply,
                      redisContext* _rc, const std::string& _cmd);  
  bool CheckStatusOK(redisReply* _preply, 
                     redisContext* _rc, const std::string& _cmd);  
  bool GetOneStringFromArr(std::string& _ret, redisReply* _reply);
  void CmdToString(const std::string& _cmd_str);
};
//
#endif
