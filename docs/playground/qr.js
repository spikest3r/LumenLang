function chunkBytes(data, size)
{
    const chunks = [];

    for (let i = 0; i < data.length; i += size)
    {
        chunks.push(
            data.slice(i, i + size)
        );
    }

    return chunks;
}

function crc32(data)
{
    let crc = 0xffffffff;

    for (let byte of data)
    {
        crc ^= byte;

        for (let i = 0; i < 8; i++)
        {
            crc =
                (crc >>> 1) ^
                (crc & 1 ? 0xedb88320 : 0);
        }
    }

    return (~crc) >>> 0;
}

function toBase64(bytes)
{
    let binary = "";

    for (const b of bytes)
        binary += String.fromCharCode(b);

    return btoa(binary);
}


/*
    wasm:
        getLmnbin() -> uint8_t*
        getLmnbinSize() -> size_t

    wasmMemory -> WebAssembly.Memory
*/
export async function exportLmnbinQR(
    wasm,
    wasmMemory,
    programName,
    chunkSize = 500
)
{
    const ptr =
        wasm._getLmnbin();

    const size =
        wasm._getLmnbinSize();

    // Copy from WASM memory
    const lmnbin =
        new Uint8Array(
            wasmMemory.buffer,
            ptr,
            size
        ).slice();

    const crc =
        crc32(lmnbin);

    const chunks =
        chunkBytes(
            lmnbin,
            chunkSize
        );

    const images = [];

    for (let i = 0; i < chunks.length; i++)
    {
        const payload =
        {
            n: programName,
            i: i,
            t: chunks.length,
            sz: lmnbin.length,
            crc: crc,
            d: toBase64(chunks[i])
        };

        const text =
            JSON.stringify(payload);

        const image =
            await QRCode.toDataURL(
                text,
                {
                    errorCorrectionLevel: "M",
                    margin: 4,
                    width: 512
                }
            );

        images.push(image);
    }

    return images;
}