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
    return parser

async def main():
    """Connect to an ESPHome device and wait for state changes."""
    api = aioesphomeapi.APIClient(namespace.ip[0], 6053, namespace.pwd[0])

    try:
        await api.connect(login=True)
    except aioesphomeapi.InvalidAuthAPIError as e:
        return print(e)

    print(api.api_version)

    async def display_off():
        await api.execute_service(
            service,
            data={
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x01, 0x00, 0x0F, 0x00, 0x01, 0x01, 0x97, 0xE0, 0x00, 0x20, 0x00, 0xC0, 0x00, 0x00, 0x20, 0x00, 0x10, 0x00, 0x00],
            }
        )

    async def display_on():
        await api.execute_service(
            service,
            data={
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x01, 0x00, 0x0F, 0x00, 0x01, 0x01, 0x97, 0xE0, 0x00, 0x20, 0x00, 0xC0, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00],
            }
        )

    async def ac_enable():
        await api.execute_service(
            service,
            data={
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x00, 0x87, 0xE0, 0x2F, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00],
            }
        )

    async def ac_disable():
        await api.execute_service(
            service,
            data={
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x00, 0x87, 0xE0, 0x2F, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
            }
        )

    async def ac_get11_01():
        await api.execute_service(
            service,
            data={
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x02, 0x00, 0x11, 0x01],
            }
        )

    async def ac_get11_00():
        await api.execute_service(
            service,
            data={
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x02, 0x00, 0x11, 0x00],
            }
        )

    async def ac_set_vlouver(lvr):
        await api.execute_service(
            service,
            data={
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x01,  lvr, 0xE0, 0x00, 0x20, 0x00, 0xC0, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00],
            }
        )

    async def ac_set_hlouver(lvr):
        await api.execute_service(
            service,
            data={
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x01, 0x97,  lvr, 0x00, 0x20, 0x00, 0xC0, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00],
            }
        )

    # key надо искать в выводе list_entities_services
    service = aioesphomeapi.UserService(
        name="send_data",
        key=311254518,
        args=[
            aioesphomeapi.UserServiceArg(name="data_buf", type=aioesphomeapi.UserServiceArgType.INT_ARRAY),
        ],
    )

    time.sleep(7)
    await ac_get11_00()
    time.sleep(7)
    await ac_get11_01()

    #await ac_set_vlouver( 0b10010000 )  # swing on
    #await ac_set_vlouver( 0b10010111 )   # swing off
    #await ac_set_vlouver( 0b10010001 )   #  1
    #await ac_set_vlouver( 0b10010010 )   #   2
    #await ac_set_vlouver( 0b10010011 )   #   3
    #await ac_set_vlouver( 0b10010100 )   #   4
    #await ac_set_vlouver( 0b10010101 )   #   5
    #await ac_set_vlouver( 0b10010110 )   # не работает, сбрасывает на swing on
    #time.sleep(5)

    #await ac_set_hlouver( 0b00000000 )  # swing on
    #await ac_set_hlouver( 0b11100000 )   # swing off
    #await ac_set_hlouver( 0b00100000 )   # не работает, сбрасывает в swing off
    #await ac_set_hlouver( 0b01000000 )   # не работает, сбрасывает в swing off
    #await ac_set_hlouver( 0b01100000 )   # не работает, сбрасывает в swing off
    #await ac_set_hlouver( 0b10000000 )   # не работает, сбрасывает в swing off
    #await ac_set_hlouver( 0b10100000 )   # не работает, сбрасывает в swing off
    #await ac_set_hlouver( 0b11000000 )   # не работает, сбрасывает в swing off
    #time.sleep(5)

    async def test_byte(bt):
        await api.execute_service(
            service,
            data={
                #display on
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                #"data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x01, 0x97, 0xE0, 0x00, 0x20, 0x00, 0xC0, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00],
                #display off
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                "data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x01, 0x97, 0xE0, 0x00, 0x20, 0x00, 0xC0, 0x00, 0x00, 0x20, 0x00, 0x10, 0x00, 0x00],
                # swing on
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                #"data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x01,  0x90, 0xE0, 0x00, 0x20, 0x00, 0xC0, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00],
                # swing off
                #               0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22
                #"data_buf": [0xBB, 0x00, 0x06, 0x80, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x01,  0x97, 0xE0, 0x00, 0x20, 0x00, 0xC0, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00],
            }
        )
    '''
    не проходит команда, если байт 1 или 7 не 0x00
    не проходит команда, если байт 3 не 0x80

    проходит и не меняется, если меняю байт 4 или 5
    '''

    #await test_byte(0b10000110)
    #await test_byte(0b01000110)
    #await test_byte(0b00100110)
    #await test_byte(0b00010110)
    time.sleep(2)


parser = createParser()
namespace = parser.parse_args()
print("IP: ", namespace.ip[0])


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