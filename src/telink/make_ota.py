import struct
from dataclasses import dataclass
from typing import Callable

import click

OTA_HDR_STRUCT = struct.Struct("<I5HIH32sI")

OTA_SUB_ELEMENT_HDR_STRUCT = struct.Struct("<HI")


@dataclass
class OTAHeader:
    """
    Header structure:
    Offset  Size    Name
    0       4       Upgrade File Identifier
    4       2       Header Version
    6       2       Header Length
    8       2       Field Control
    10      2       Manufacturer Code
    12      2       Image Type
    14      4       File Version
    18      2       Zigbee Stack Version
    20      32      Header String
    52      4       Total Image Size
    """

    upgrade_file_identifier: int = 0xBEEF11E  # Zigbee OTA magic number
    header_version: int = 0x100  # Version 1.0
    header_length: int = 56  # Fixed size
    field_control: int = 0  # No optional fields
    manufacturer_code: int = 0x1141  # Telink Manufacturer Code
    image_type: int = 0x0000  # Device specific image type, should be set by user
    file_version: int = 0x00000000  # Should be set by user
    zigbee_stack_version: int = 2  # Zigbee stack version
    header_string: bytes = b"NOT_SET"  # Should be set by user, max 31 bytes
    total_image_size: int = 0  # Should be set by user

    def pack(self) -> bytes:
        return OTA_HDR_STRUCT.pack(
            self.upgrade_file_identifier,
            self.header_version,
            self.header_length,
            self.field_control,
            self.manufacturer_code,
            self.image_type,
            self.file_version,
            self.zigbee_stack_version,
            self.header_string + b"\x00" * (32 - len(self.header_string)),
            self.total_image_size,
        )


@dataclass
class OTASubElementHeader:
    """
    Sub-element header structure:
    Offset  Size    Name
    0       2       Sub-element ID
    2       4       Sub-element Length
    """

    sub_element_id: int
    sub_element_length: int

    def pack(self) -> bytes:
        return OTA_SUB_ELEMENT_HDR_STRUCT.pack(
            self.sub_element_id, self.sub_element_length
        )


def make_ota_image(
    image_data: bytes,
    manufacturer_id: int,
    image_type: int,
    file_version: int | None,
    header_string: str,
) -> bytes:
    hs = str.encode(header_string)
    if len(hs) > 31:
        print("Error: header_string size is too long!")
        exit(2)

    total_image_size = (
        OTA_HDR_STRUCT.size + OTA_SUB_ELEMENT_HDR_STRUCT.size + len(image_data)
    )
    if file_version is None:
        file_version = int.from_bytes(image_data[2:6], byteorder="little")

    ota_header = OTAHeader(
        manufacturer_code=manufacturer_id,
        image_type=image_type,
        file_version=file_version,
        header_string=hs,
        total_image_size=total_image_size,
    )
    sub_element_header = OTASubElementHeader(
        sub_element_id=0, sub_element_length=len(image_data)
    )

    return ota_header.pack() + sub_element_header.pack() + image_data


@click.group()
def cli():
    pass


def max_length_validator(
    max_length: int,
) -> Callable[[click.Context, click.Parameter, str | None], str | None]:
    """Factory to create a validator that checks for maximum string length."""

    def validate(
        ctx: click.Context,
        param: click.Parameter,
        value: str | None,
    ) -> str | None:
        if value is not None and len(value) > max_length:
            raise click.BadParameter(
                f"must be at most {max_length} characters long", ctx=ctx, param=param
            )
        return value

    return validate


class IntOrHexParamType(click.ParamType):
    name = "int-or-hex"

    def convert(
        self, value: str, param: click.Parameter | None, ctx: click.Context | None
    ) -> int:
        try:
            # Handles decimal, hex (0x...), octal (0o...), binary (0b...)
            return int(value, 0)
        except ValueError:
            self.fail(
                f"{value!r} is not a valid integer (supports 0x.., 0o.., 0b.., or decimal)",
                param,
                ctx,
            )


INT_OR_HEX = IntOrHexParamType()


@cli.command()
@click.argument("input_file", type=click.Path(exists=True))
@click.argument("output_file", type=click.Path())
@click.option(
    "--manufacturer-id",
    type=INT_OR_HEX,
    default="0x1141",
    help="Manufacturer ID (e.g., 0x1141 for Telink)",
)
@click.option(
    "--image-type", type=INT_OR_HEX, required=True, help="Image Type (device specific)"
)
@click.option(
    "--file-version",
    type=INT_OR_HEX,
    required=False,
    default=None,
    help="File Version (e.g., 0x00000001), uses value from firmware if not set",
)
@click.option(
    "--header-string",
    type=str,
    default="Telink OTA Image",
    callback=max_length_validator(31),
    help="Header String (max 31 characters)",
)
def create_ota(
    input_file: str,
    output_file: str,
    manufacturer_id: int,
    image_type: int,
    file_version: int | None,
    header_string: str,
) -> None:
    with open(input_file, "rb") as f:
        image_data = f.read()
    ota_image = make_ota_image(
        image_data,
        manufacturer_id,
        image_type,
        file_version,
        header_string,
    )
    with open(output_file, "wb") as f:
        f.write(ota_image)


if __name__ == "__main__":
    cli()
