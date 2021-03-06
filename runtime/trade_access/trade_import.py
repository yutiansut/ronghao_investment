#!/usr/bin/env python2
#-*- coding: utf-8 -*-

"""
每天早上五点会将前一天所有策略产生的买卖点均导入sqlite数据库（trade.db）中的order_ins表
"""

__author__ = 'ronghao dev'

import os, re, sys
import datetime
import time
import sqlite3
import ConfigParser

def get_yesterday():
    yesterday = datetime.datetime.now()-datetime.timedelta(days=1)
    return yesterday.strftime('%Y%m%d')

def sleep_until_tomorrow_morning():
    ed_time = datetime.time(5, 0, 0)
    tomorrow = datetime.datetime.now()+datetime.timedelta(days=1)
    spec_dt = datetime.datetime.combine(tomorrow.date(), ed_time)
    sleep_sec = int((spec_dt-datetime.datetime.now()).total_seconds())
    time.sleep(sleep_sec)
                
class trade_import:
    def __init__(self):
        conf = ConfigParser.ConfigParser()
        conf.read('ini/config.ini')
        self._dbname = conf.get('database', 'db_name')
        print '当前连接的数据库名称为：', self._dbname
        
        self._root_dir = '../strategy_server/database_import'
        self._index = [0, 1, 2, 3, 6, 9, 10, 11, 12]
        self._exec_sqlite_trans(self._create_trade_table)
        
    def _exec_sqlite_trans(self, callback):
        conn = sqlite3.connect(self._dbname)
        conn.text_factory = str
        cursor = conn.cursor()
        if callable(callback):
            callback(cursor)
        else:
            raise AttributeError('_exec_sqlite_trans need a callable arg')
        cursor.close()
        conn.commit()
        conn.close()        
        
    def _create_trade_table(self, cursor):
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS order_ins    --买卖点导入表
        (local_datetime BIGINT NOT NULL,            --本地日期时间
        stg_id INT NOT NULL,                                --策略id
        src_id INT NOT NULL,                                --下单源id
        trade_seq INT NOT NULL,                          --交易流水号
        ins_id VARCHAR(16) NOT NULL,                 --合约id
        dia_type VARCHAR(16) NOT NULL,             --图表类型
        dia_seq INT NOT NULL,                              --图表流水号
        offset_flag VARCHAR(8) NOT NULL,            --开平标志
        direction VARCHAR(8) NOT NULL,              --买卖方向
        price INT NOT NULL,                                  --价格
        vol INT NOT NULL,                                     --手数
        level INT NOT NULL,                                  --信号
        market_datetime BIGINT NOT NULL);        --行情日期时间
        """)       
        
    def _import_all_file(self, cursor):
        for file in self._file_list:
            print '当前正在导入买卖点的文件是：', file
            with open(file, 'r') as f:
                for line in f.readlines():
                    print '当前导入的记录是：', line[:-1]
                    field_list = re.split('[#\s]\s*', line)
                    field_list[0] = field_list[0][6:]       #去掉'本地'
                    field_list[-3] = field_list[-3][6:]    #去掉'行情' 
                    field_list.pop(-1)      #丢掉最后一个换行符
                    
                    #将日期时间绑定在一起存储
                    field_list[0] = ''.join(field_list[0:2])
                    field_list[-2] = ''.join(field_list[-2:])
                    field_list.pop(1)
                    field_list.pop(-1)
                    
                    #类型转换
                    for i, value in enumerate(field_list):
                        if i in self._index:
                            field_list[i] = int(value)
                            
                    cursor.execute('INSERT INTO order_ins VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)', field_list)
        
    def import_yesterday_file(self):
        file_dir = os.path.join(self._root_dir, get_yesterday())
        if os.path.exists(file_dir):
            self._file_list = os.listdir(file_dir)
            for i, file in enumerate(self._file_list):
                self._file_list[i] = os.path.join(file_dir, file)
            self._exec_sqlite_trans(self._import_all_file)

def cpp_main(*args):
    while True:
        sleep_until_tomorrow_morning()
        trade_import().import_yesterday_file()

def cancel_import(*args):
    sys.exit(0)
    
def create_deamon():
    # fork进程        
    try:
        if os.fork() > 0: 
            os._exit(0)
    except OSError, error:
        print 'fork #1 failed: %d (%s)' % (error.errno, error.strerror)
        os._exit(1)    
#     os.chdir('/')
    os.setsid()
    os.umask(0)
    try:
        pid = os.fork()
        if pid > 0:
            print 'Daemon PID %d' % pid
            os._exit(0)
    except OSError, error:
        print 'fork #2 failed: %d (%s)' % (error.errno, error.strerror)
        os._exit(1)
        
    # 重定向标准IO
    sys.stdout.flush()
    sys.stderr.flush()
    si = file("/dev/null", 'r')
    so = file("/dev/null", 'a+')
    se = file("/dev/null", 'a+', 0)
    os.dup2(si.fileno(), sys.stdin.fileno())
    os.dup2(so.fileno(), sys.stdout.fileno())
    os.dup2(se.fileno(), sys.stderr.fileno())

    # 在子进程中执行代码
    cpp_main() # function demo

if __name__ == '__main__':
    create_deamon()
