import argparse
import struct
import can

DEFAULT_BITRATE = 500000

BMS_PROTECTION_ID = 0x010
BMS_CELL_VOLTAGE_MIN_ID = 0x250
BMS_CELL_VOLTAGE_MAX_ID = 0x251
BMS_CELL_TEMPERATURE_MIN_ID = 0x252
BMS_CELL_TEMPERATURE_MAX_ID = 0x253
BMS_PACK_ELECTRICAL_ID = 0x254
BMS_PACK_POWER_ID = 0x255


def f32_le(data, offset=0):
    return struct.unpack_from("<f", bytes(data), offset)[0]


def decode(msg):
    can_id = msg.arbitration_id
    data = msg.data

    if can_id == BMS_PROTECTION_ID and len(data) >= 1:
        b = data[0]
        print(
            f"0x{can_id:03X} BMS_Protection: "
            f"UT={(b >> 0) & 1} "
            f"OT={(b >> 1) & 1} "
            f"UV={(b >> 2) & 1} "
            f"OV={(b >> 3) & 1} "
            f"OCD={(b >> 4) & 1}"
        )

    elif can_id == BMS_CELL_VOLTAGE_MIN_ID and len(data) >= 5:
        print(f"0x{can_id:03X} BMS_CellVoltageMin: CellVoltMinID={data[0]} CellVoltMin={f32_le(data, 1):.3f} V")

    elif can_id == BMS_CELL_VOLTAGE_MAX_ID and len(data) >= 5:
        print(f"0x{can_id:03X} BMS_CellVoltageMax: CellVoltMaxID={data[0]} CellVoltMax={f32_le(data, 1):.3f} V")

    elif can_id == BMS_CELL_TEMPERATURE_MIN_ID and len(data) >= 5:
        print(f"0x{can_id:03X} BMS_CellTemperatureMin: CellTempMinID={data[0]} CellTempMin={f32_le(data, 1):.3f} C")

    elif can_id == BMS_CELL_TEMPERATURE_MAX_ID and len(data) >= 5:
        print(f"0x{can_id:03X} BMS_CellTemperatureMax: CellTempMaxID={data[0]} CellTempMax={f32_le(data, 1):.3f} C")

    elif can_id == BMS_PACK_ELECTRICAL_ID and len(data) >= 8:
        print(f"0x{can_id:03X} BMS_PackElectrical: PackCurrent={f32_le(data, 0):.3f} A PackVoltage={f32_le(data, 4):.3f} V")

    elif can_id == BMS_PACK_POWER_ID and len(data) >= 4:
        print(f"0x{can_id:03X} BMS_PackPower: PackPower={f32_le(data, 0):.3f} W\n")

    else:
        print(msg)


def parse_args():
    parser = argparse.ArgumentParser(description="Decode BMS CAN messages from a vulCAN/slcan adapter.")
    parser.add_argument("-c", "--channel", required=True, help="Serial channel, e.g. /dev/tty.usbmodem, /dev/ttyACM0, COM3")
    parser.add_argument("-b", "--bitrate", type=int, default=DEFAULT_BITRATE, help=f"CAN bitrate, default {DEFAULT_BITRATE}")
    parser.add_argument("-i", "--interface", default="slcan", help="python-can interface, default slcan")
    return parser.parse_args()


def main():
    args = parse_args()

    bus = can.interface.Bus(
        interface=args.interface,
        channel=args.channel,
        bitrate=args.bitrate,
    )

    print(f"Listening for BMS CAN messages on {args.channel} at {args.bitrate} bps...")

    try:
        while True:
            msg = bus.recv()
            if msg is not None:
                decode(msg)
    except KeyboardInterrupt:
        print("\nAborted.")
    finally:
        bus.shutdown()


if __name__ == "__main__":
    main()
