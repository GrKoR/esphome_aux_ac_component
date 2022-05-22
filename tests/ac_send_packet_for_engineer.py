import time
import aioesphomeapi
import asyncio
import re
import sys
import argparse
from aioesphomeapi.api_pb2 import (LOG_LEVEL_NONE,
                                   LOG_LEVEL_ERROR,
                                   LOG_LEVEL_WARN,
                                   LOG_LEVEL_INFO,
                                   LOG_LEVEL_DEBUG,
                                   LOG_LEVEL_VERBOSE,
                                   LOG_LEVEL_VERY_VERBOSE)

def createParser ():
    parser = argparse.ArgumentParser(
        description='''This script is used for collecting logs from ac_aux ESPHome component.
                       For more info, see https://github.com/GrKoR/ac_python_logger''',
        add_help = False)
    parent_group = parser.add_argument_group (title='Params')
    parent_group.add_argument ('--help', '-h', action='help', help='show this help message and exit')
    parent_group.add_argument ('-i', '--ip', nargs=1, required=True, help='IP address of the esphome device')
    parent_group.add_argument ('-p', '--pwd', nargs=1, required=True, help='native API password for the esphome device')
    parent_group.add_argument ('-n', '--name', nargs=1, default=['noname'], help='name of this devace in the log')
    parent_group.add_argument ('-l', '--logfile', nargs=1, default=['%4d-%02d-%02d %02d-%02d-%02d log.csv' % time.localtime()[0:6]], help='log file name')
    return parser

async def main():
    """Connect to an ESPHome device and wait for state changes."""
    api = aioesphomeapi.APIClient(namespace.ip[0], 6053, namespace.pwd[0])

    try:
        await api.connect(login=True)
    except aioesphomeapi.InvalidAuthAPIError as e:
        return print(e)

    print(api.api_version)

    def log_AC(isAirConLog):
        parts = re.search("(\d{10}): (\[\S{2}\]) \[([0-9A-F ]{23})\]\s?((?:[0-9A-F]{2}\s*)*) \[([0-9A-F ]{5})\]", isAirConLog.group(1))
        packString = '\n' + namespace.name[0]
        packString += ";" + "%4d-%02d-%02d %02d:%02d:%02d" % time.localtime()[0:6]
        """millis of message"""
        packString += ";" + parts.group(1)
        """direction"""
        packString += ";" + parts.group(2)
        """header"""
        packString += ";" + ';'.join(parts.group(3).split(" "))
        """body (may be void)"""
        if len(parts.group(4)) > 0:
            packString += ";" + ';'.join(parts.group(4).split(" "))
        """crc"""
        packString += ";" + ';'.join(parts.group(5).split(" "))
        print(packString)
        with open(namespace.logfile[0], 'a+') as file:
            file.write( packString )

    def log_Dallas(isDallasLog):
        parts = re.search("'([\w ]+)': Got Temperature=([-]?\d+\.\d+)°C", isDallasLog.group(1))
        packString = '\n' + parts.group(1)
        packString += ";" + "%4d-%02d-%02d %02d:%02d:%02d" % time.localtime()[0:6]
        """millis of message always empty"""
        packString += ";"
        """direction"""
        packString += ";[<=]"
        """additional data flag"""
        packString += ";AA"
        """dallas temperature"""
        packString += ";" + parts.group(2)
        print(packString)
        with open(namespace.logfile[0], 'a+') as file:
            file.write( packString )

    def log_callback(log):
        """Print the log for AirCon"""
        isAirConLog = re.search("\[AirCon:\d+\]: (.+\])", log.message.decode('utf-8'))
        if isAirConLog:
            log_AC(isAirConLog)
        if namespace.logdallas:
            isDallasLog = re.search("\[dallas.sensor:\d+\]: (.+C)", log.message.decode('utf-8'))
            if isDallasLog:
                log_Dallas(isDallasLog)


    # Subscribe to the log
    # await api.subscribe_logs(log_callback, LOG_LEVEL_DEBUG)

    # print(await api.device_info())
    print(f"%s" % (await api.list_entities_services(),))

    # key надо искать в выводе list_entities_services
    service = aioesphomeapi.UserService(
        name="send_data",
        key=311254518,
        args=[
            aioesphomeapi.UserServiceArg(name="data_buf", type=aioesphomeapi.UserServiceArgType.INT_ARRAY),
        ],
    )

    await api.execute_service(
        service,
        data={
            # display off
            "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x01, 0x7F, 0xE0, 0x00, 0x20, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00],
        }
    )

    time.sleep(3)

    await api.execute_service(
        service,
        data={
            # display on
            "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x01, 0x7F, 0xE0, 0x00, 0x20, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
        }
    )

parser = createParser()
namespace = parser.parse_args()
print(namespace.name[0], namespace.ip[0])


loop = asyncio.get_event_loop()
try:
    #asyncio.ensure_future(main())
    #loop.run_forever()
    loop.run_until_complete(main())
except aioesphomeapi.InvalidAuthAPIError as e:
    print(e)
except KeyboardInterrupt:
    pass
finally:
    loop.close()
    pass