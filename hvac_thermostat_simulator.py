import struct
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox

import serial
from serial.tools import list_ports


# ============================================================
# CONFIGURATION
# ============================================================

BAUD = 115200

COMMAND_PACKET_SIZE = 6
STATUS_PACKET_SIZE = 14


# ============================================================
# HVAC COMMANDS
# Must match hvac_states.h
# ============================================================

CMD_OFF = 0
CMD_HEAT = 1
CMD_HEAT_OFF = 2
CMD_FAN_ON = 3
CMD_FAN_AUTO = 4


COMMAND_NAMES = {
    CMD_OFF: "OFF",
    CMD_HEAT: "HEAT",
    CMD_HEAT_OFF: "HEAT OFF",
    CMD_FAN_ON: "FAN ON",
    CMD_FAN_AUTO: "FAN AUTO",
}


# ============================================================
# HVAC STATES
# Must match hvac_states.h
# ============================================================

STATE_NAMES = {
    0: "IDLE",
    1: "FAN CIRCULATE",
    2: "IGNITION",
    3: "WARMUP",
    4: "VERIFY RPM",
    5: "RUNNING",
    6: "COOLDOWN",
    7: "FAULT",
}


# ============================================================
# HVAC FAULTS
# Must match hvac_states.h
# ============================================================

FAULT_NAMES = {
    0: "NONE",
    1: "FLAME",
    2: "FAN",
    3: "SAFETY",
}


# ============================================================
# CHECKSUM
# ============================================================

def checksum(data):
    """
    XOR checksum.

    Must match calculate_checksum()
    in hvac_comms.c.
    """

    value = 0

    for byte in data:
        value ^= byte

    return value


# ============================================================
# CREATE COMMAND PACKET
# ============================================================

def make_command(command):
    """
    Creates the 6-byte thermostat packet:

        BYTE 0      Command
        BYTE 1-4    Timestamp
        BYTE 5      Checksum

    Timestamp is BIG-ENDIAN to match the ESP32.
    """

    timestamp = int(
        time.time() * 1_000_000
    ) & 0xFFFFFFFF

    data = struct.pack(
        ">BI",
        command,
        timestamp
    )

    packet = data + bytes([
        checksum(data)
    ])

    return packet


# ============================================================
# DECODE STATUS PACKET
# ============================================================

def decode_status(packet):
    """
    Decodes the 14-byte HVAC status packet.

    Packet:

        BYTE 0       State
        BYTE 1       Fault
        BYTE 2       Fan
        BYTE 3       Heater
        BYTE 4-7     Timestamp
        BYTE 8-9     Flame ADC
        BYTE 10-11   Fan RPM
        BYTE 12      Reserved
        BYTE 13      Checksum
    """

    if len(packet) != STATUS_PACKET_SIZE:
        raise ValueError(
            f"Expected {STATUS_PACKET_SIZE} bytes, "
            f"got {len(packet)}"
        )

    # --------------------------------------------------------
    # Check checksum
    # --------------------------------------------------------

    calculated = checksum(
        packet[:13]
    )

    received = packet[13]

    if calculated != received:

        raise ValueError(
            "Checksum error "
            f"RX=0x{received:02X} "
            f"CALC=0x{calculated:02X}"
        )

    # --------------------------------------------------------
    # Decode single-byte values
    # --------------------------------------------------------

    state = packet[0]

    fault = packet[1]

    fan = packet[2]

    heater = packet[3]

    # --------------------------------------------------------
    # Timestamp
    # BIG-ENDIAN
    # --------------------------------------------------------

    timestamp = struct.unpack(
        ">I",
        packet[4:8]
    )[0]

    # --------------------------------------------------------
    # Flame ADC
    # BIG-ENDIAN
    # --------------------------------------------------------

    flame = struct.unpack(
        ">H",
        packet[8:10]
    )[0]

    # --------------------------------------------------------
    # RPM
    # BIG-ENDIAN
    # --------------------------------------------------------

    rpm = struct.unpack(
        ">H",
        packet[10:12]
    )[0]

    return (
        state,
        fault,
        fan,
        heater,
        timestamp,
        flame,
        rpm
    )


# ============================================================
# HVAC SIMULATOR
# ============================================================

class HVACSimulator:

    def __init__(self, root):

        self.root = root

        self.root.title(
            "HVAC Thermostat Simulator"
        )

        self.root.geometry(
            "900x750"
        )

        self.root.minsize(
            800,
            650
        )

        # ----------------------------------------------------
        # Serial
        # ----------------------------------------------------

        self.ser = None

        self.running = True

        self.receive_thread = None

        self.port = tk.StringVar()

        self.connection = tk.StringVar(
            value="DISCONNECTED"
        )

        # ----------------------------------------------------
        # Status variables
        # ----------------------------------------------------

        self.state = tk.StringVar(
            value="---"
        )

        self.fault = tk.StringVar(
            value="---"
        )

        self.fan = tk.StringVar(
            value="---"
        )

        self.heater = tk.StringVar(
            value="---"
        )

        self.flame = tk.StringVar(
            value="---"
        )

        self.rpm = tk.StringVar(
            value="---"
        )

        self.timestamp = tk.StringVar(
            value="---"
        )

        self.last_command = tk.StringVar(
            value="---"
        )

        self.packet_count = 0

        # ----------------------------------------------------
        # Build GUI
        # ----------------------------------------------------

        self.build()

        self.refresh_ports()

        # ----------------------------------------------------
        # Close handler
        # ----------------------------------------------------

        self.root.protocol(
            "WM_DELETE_WINDOW",
            self.close
        )


    # ========================================================
    # BUILD GUI
    # ========================================================

    def build(self):

        # ----------------------------------------------------
        # Title
        # ----------------------------------------------------

        title = ttk.Label(
            self.root,
            text="HVAC THERMOSTAT SIMULATOR",
            font=("Segoe UI", 20, "bold")
        )

        title.pack(
            pady=15
        )


        # ====================================================
        # UART CONNECTION
        # ====================================================

        connection = ttk.LabelFrame(
            self.root,
            text="UART CONNECTION",
            padding=10
        )

        connection.pack(
            fill="x",
            padx=20,
            pady=5
        )


        ttk.Label(
            connection,
            text="COM Port:"
        ).grid(
            row=0,
            column=0,
            padx=5,
            pady=5
        )


        self.port_box = ttk.Combobox(
            connection,
            textvariable=self.port,
            width=15,
            state="readonly"
        )

        self.port_box.grid(
            row=0,
            column=1,
            padx=5
        )


        ttk.Button(
            connection,
            text="Refresh",
            command=self.refresh_ports
        ).grid(
            row=0,
            column=2,
            padx=5
        )


        ttk.Button(
            connection,
            text="Connect",
            command=self.connect
        ).grid(
            row=0,
            column=3,
            padx=5
        )


        ttk.Button(
            connection,
            text="Disconnect",
            command=self.disconnect
        ).grid(
            row=0,
            column=4,
            padx=5
        )


        ttk.Label(
            connection,
            textvariable=self.connection,
            font=("Segoe UI", 10, "bold")
        ).grid(
            row=0,
            column=5,
            padx=15
        )


        # ====================================================
        # COMMANDS
        # ====================================================

        commands = ttk.LabelFrame(
            self.root,
            text="MANUAL HVAC COMMANDS",
            padding=15
        )

        commands.pack(
            fill="x",
            padx=20,
            pady=8
        )


        # OFF

        ttk.Button(
            commands,
            text="OFF",
            width=15,
            command=lambda:
                self.send(
                    CMD_OFF,
                    "OFF"
                )
        ).grid(
            row=0,
            column=0,
            padx=5,
            pady=5
        )


        # HEAT

        ttk.Button(
            commands,
            text="HEAT ON",
            width=15,
            command=lambda:
                self.send(
                    CMD_HEAT,
                    "HEAT"
                )
        ).grid(
            row=0,
            column=1,
            padx=5,
            pady=5
        )


        # HEAT OFF

        ttk.Button(
            commands,
            text="HEAT OFF",
            width=15,
            command=lambda:
                self.send(
                    CMD_HEAT_OFF,
                    "HEAT OFF"
                )
        ).grid(
            row=0,
            column=2,
            padx=5,
            pady=5
        )


        # FAN ON

        ttk.Button(
            commands,
            text="FAN ON",
            width=15,
            command=lambda:
                self.send(
                    CMD_FAN_ON,
                    "FAN ON"
                )
        ).grid(
            row=0,
            column=3,
            padx=5,
            pady=5
        )


        # FAN AUTO

        ttk.Button(
            commands,
            text="FAN AUTO",
            width=15,
            command=lambda:
                self.send(
                    CMD_FAN_AUTO,
                    "FAN AUTO"
                )
        ).grid(
            row=0,
            column=4,
            padx=5,
            pady=5
        )


        # ====================================================
        # STATUS
        # ====================================================

        status = ttk.LabelFrame(
            self.root,
            text="ESP32 HVAC STATUS",
            padding=15
        )

        status.pack(
            fill="x",
            padx=20,
            pady=8
        )


        values = [
            ("State", self.state),
            ("Fault", self.fault),
            ("Fan", self.fan),
            ("Heater", self.heater),
            ("Flame ADC", self.flame),
            ("Fan RPM", self.rpm),
            ("Timestamp", self.timestamp),
            ("Last Command", self.last_command),
        ]


        for row, (name, variable) in enumerate(values):

            ttk.Label(
                status,
                text=name + ":",
                font=(
                    "Segoe UI",
                    10,
                    "bold"
                )
            ).grid(
                row=row,
                column=0,
                sticky="w",
                padx=5,
                pady=3
            )


            ttk.Label(
                status,
                textvariable=variable
            ).grid(
                row=row,
                column=1,
                sticky="w",
                padx=15,
                pady=3
            )


        # ====================================================
        # UART LOG
        # ====================================================

        log_frame = ttk.LabelFrame(
            self.root,
            text="UART LOG",
            padding=5
        )

        log_frame.pack(
            fill="both",
            expand=True,
            padx=20,
            pady=8
        )


        self.log = tk.Text(
            log_frame,
            height=10,
            state="disabled",
            font=("Consolas", 9)
        )

        self.log.pack(
            fill="both",
            expand=True
        )


        # ----------------------------------------------------
        # Clear log
        # ----------------------------------------------------

        ttk.Button(
            self.root,
            text="Clear Log",
            command=self.clear_log
        ).pack(
            pady=5
        )


    # ========================================================
    # PORT DISCOVERY
    # ========================================================

    def refresh_ports(self):

        ports = [
            port.device
            for port in list_ports.comports()
        ]

        self.port_box["values"] = ports

        if ports:

            self.port.set(
                ports[0]
            )

        else:

            self.port.set("")


    # ========================================================
    # CONNECT
    # ========================================================

    def connect(self):

        if not self.port.get():

            messagebox.showwarning(
                "UART",
                "Select a COM port first."
            )

            return


        # Close an existing connection first

        self.disconnect()


        try:

            self.ser = serial.Serial(
                port=self.port.get(),
                baudrate=BAUD,
                timeout=0.1
            )


            self.connection.set(
                "CONNECTED"
            )


            self.log_message(
                f"Connected to {self.port.get()} "
                f"at {BAUD} baud"
            )


            # ------------------------------------------------
            # Start receive thread
            # ------------------------------------------------

            self.receive_thread = threading.Thread(
                target=self.receive_loop,
                daemon=True
            )

            self.receive_thread.start()


        except Exception as e:

            self.ser = None

            self.connection.set(
                "DISCONNECTED"
            )

            messagebox.showerror(
                "UART Error",
                str(e)
            )


    # ========================================================
    # DISCONNECT
    # ========================================================

    def disconnect(self):

        old_ser = self.ser

        self.ser = None

        if old_ser is not None:

            try:
                old_ser.close()

            except Exception:
                pass


        self.connection.set(
            "DISCONNECTED"
        )


    # ========================================================
    # SEND COMMAND
    # ========================================================

    def send(
        self,
        command,
        name
    ):

        if (
            self.ser is None
            or not self.ser.is_open
        ):

            messagebox.showwarning(
                "UART",
                "Connect to the ESP32 first."
            )

            return


        packet = make_command(
            command
        )


        try:

            self.ser.write(
                packet
            )

            self.ser.flush()


            self.last_command.set(
                name
            )


            self.log_message(
                "TX %-10s : %s"
                % (
                    name,
                    packet.hex(" ")
                )
            )


        except Exception as e:

            self.log_message(
                "TX ERROR: " + str(e)
            )


    # ========================================================
    # RECEIVE LOOP
    # ========================================================

    def receive_loop(self):

        buffer = bytearray()


        while self.running:

            try:

                ser = self.ser


                if (
                    ser is None
                    or not ser.is_open
                ):

                    time.sleep(0.05)

                    continue


                # ------------------------------------------------
                # Read available bytes
                # ------------------------------------------------

                data = ser.read(
                    32
                )


                if not data:

                    continue


                buffer.extend(
                    data
                )


                # ------------------------------------------------
                # Look for complete 14-byte packet
                # ------------------------------------------------

                while len(buffer) >= STATUS_PACKET_SIZE:

                    packet = bytes(
                        buffer[
                            :STATUS_PACKET_SIZE
                        ]
                    )


                    try:

                        status = decode_status(
                            packet
                        )


                        # Valid packet.
                        # Remove exactly 14 bytes.

                        del buffer[
                            :STATUS_PACKET_SIZE
                        ]


                        self.root.after(
                            0,
                            self.update_status,
                            status
                        )


                    except ValueError as error:

                        # Packet is not valid.
                        # Shift one byte and try again.

                        self.root.after(
                            0,
                            self.log_message,
                            "RX SYNC ERROR: "
                            + str(error)
                        )


                        del buffer[0]


            except serial.SerialException as e:

                self.root.after(
                    0,
                    self.log_message,
                    "UART ERROR: " + str(e)
                )

                break


            except Exception as e:

                self.root.after(
                    0,
                    self.log_message,
                    "RX ERROR: " + str(e)
                )

                break


    # ========================================================
    # UPDATE STATUS
    # ========================================================

    def update_status(
        self,
        status
    ):

        (
            state,
            fault,
            fan,
            heater,
            timestamp,
            flame,
            rpm
        ) = status


        # ----------------------------------------------------
        # State
        # ----------------------------------------------------

        self.state.set(
            STATE_NAMES.get(
                state,
                f"UNKNOWN ({state})"
            )
        )


        # ----------------------------------------------------
        # Fault
        # ----------------------------------------------------

        self.fault.set(
            FAULT_NAMES.get(
                fault,
                f"UNKNOWN ({fault})"
            )
        )


        # ----------------------------------------------------
        # Fan
        # ----------------------------------------------------

        self.fan.set(
            "ON"
            if fan
            else "OFF"
        )


        # ----------------------------------------------------
        # Heater
        # ----------------------------------------------------

        self.heater.set(
            "ON"
            if heater
            else "OFF"
        )


        # ----------------------------------------------------
        # Flame
        # ----------------------------------------------------

        self.flame.set(
            str(flame)
        )


        # ----------------------------------------------------
        # RPM
        # ----------------------------------------------------

        self.rpm.set(
            str(rpm)
        )


        # ----------------------------------------------------
        # Timestamp
        # ----------------------------------------------------

        self.timestamp.set(
            str(timestamp)
        )


        # ----------------------------------------------------
        # Packet counter
        # ----------------------------------------------------

        self.packet_count += 1


        # ----------------------------------------------------
        # Log
        # ----------------------------------------------------

        self.log_message(
            "RX STATUS | "
            f"state={self.state.get()} | "
            f"fault={self.fault.get()} | "
            f"fan={self.fan.get()} | "
            f"heater={self.heater.get()} | "
            f"flame={flame} | "
            f"rpm={rpm}"
        )


    # ========================================================
    # LOG MESSAGE
    # ========================================================

    def log_message(
        self,
        message
    ):

        # Make sure GUI update happens safely

        try:

            self.log.config(
                state="normal"
            )


            self.log.insert(
                "end",
                "[%s] %s\n"
                % (
                    time.strftime(
                        "%H:%M:%S"
                    ),
                    message
                )
            )


            self.log.see(
                "end"
            )


            self.log.config(
                state="disabled"
            )

        except tk.TclError:

            pass


    # ========================================================
    # CLEAR LOG
    # ========================================================

    def clear_log(self):

        self.log.config(
            state="normal"
        )

        self.log.delete(
            "1.0",
            "end"
        )

        self.log.config(
            state="disabled"
        )


    # ========================================================
    # CLOSE
    # ========================================================

    def close(self):

        self.running = False

        self.disconnect()

        self.root.destroy()


# ============================================================
# MAIN
# ============================================================

if __name__ == "__main__":

    root = tk.Tk()

    app = HVACSimulator(
        root
    )

    root.mainloop()