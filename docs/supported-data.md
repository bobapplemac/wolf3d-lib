# Supported game data

wolf3d-lib requires original Wolfenstein 3D data and does not distribute it.
The first release is validated against the following exact v1.4 sets. Filename
matching is case-insensitive; SHA-256 values identify the file contents.

## Apogee shareware v1.4 (`WL1`)

| File | SHA-256 |
| --- | --- |
| `AUDIOHED.WL1` | `39351624ae6f8eef4b873e060c1a6f3e5ee7e81c4939c485275a89b145336338` |
| `AUDIOT.WL1` | `1e2c9ae30398a14c61a4ddd39aabaa0dbcc984cc4924a3e51df75574c259cfb9` |
| `GAMEMAPS.WL1` | `a6a6654b342f2c027bcb22bfce0a41f9fc0063b775e9e4da0c771970e53e11aa` |
| `MAPHEAD.WL1` | `3458f661c9b875bca99ea22a7267771fad8f2a33c699ea732cf7c3322909bf8c` |
| `VGADICT.WL1` | `59878fec65f033b00dbb1240317d1793f5858213dc8013b4d68f9ac8b45b0c80` |
| `VGAGRAPH.WL1` | `d5176f843c53415132db199c19f38591eaf3cedd35a4f7eb2698c1865d83030d` |
| `VGAHEAD.WL1` | `f4cc800dc8444373092d4eaa5d6ab59d63d23a510a9d38b73ca9b3dbb700d18b` |
| `VSWAP.WL1` | `698f217257e2cbb951a4d110ba09140291f38d0121b3784d1d6be59c03a6b47b` |

## Apogee full v1.4 (`WL6`)

| File | SHA-256 |
| --- | --- |
| `AUDIOHED.WL6` | `16e21eab17af2062019cc85cc271f887191301d1aa6de04b1afac5998aad9d9c` |
| `AUDIOT.WL6` | `2cc23cb811df16e656f1fea25cd2629859c1ec9997d35bc3b1776594094b67ef` |
| `GAMEMAPS.WL6` | `3df9f2ad54c601e79ab117c8175477b5d96571ba9e290cb9cd7d910abfeaae56` |
| `MAPHEAD.WL6` | `289e04f47128a5ba19f9b3f912b4048e26dc16c6fa00205ead51efb1d8e23c69` |
| `VGADICT.WL6` | `e4ca6e61a1da1de5b4b59b75fbd702f173238577b2f870e679fd499cfe78bf00` |
| `VGAGRAPH.WL6` | `84adea791e3ab1251312ee159414e48d5114622dc84348cebbac8a1dacb4b41f` |
| `VGAHEAD.WL6` | `386b56a62ce79cdfa502542b0c9a9bad0d29fab4250edf17e05ee4ac51aa37a8` |
| `VSWAP.WL6` | `49ba24e0b3916732cd065122de4fe6fb6e6a5009c353eafa407c0e3a5a503407` |

## GT/ID/Activision full v1.4 (`WL6`)

| File | SHA-256 |
| --- | --- |
| `AUDIOHED.WL6` | `16e21eab17af2062019cc85cc271f887191301d1aa6de04b1afac5998aad9d9c` |
| `AUDIOT.WL6` | `2cc23cb811df16e656f1fea25cd2629859c1ec9997d35bc3b1776594094b67ef` |
| `GAMEMAPS.WL6` | `3df9f2ad54c601e79ab117c8175477b5d96571ba9e290cb9cd7d910abfeaae56` |
| `MAPHEAD.WL6` | `289e04f47128a5ba19f9b3f912b4048e26dc16c6fa00205ead51efb1d8e23c69` |
| `VGADICT.WL6` | `4411cbdb446af98392cf685a583d3266e644edcff149807fab0ae5850c54586c` |
| `VGAGRAPH.WL6` | `0f7a038b8523d729eb98fe0bc50a858c47a41569e07cbbbf5d8aca8ff4e9863e` |
| `VGAHEAD.WL6` | `a9823004be77b68813f0e4d9ac35947e2e2586271fbc2fb0685ff8ecb1deb3d5` |
| `VSWAP.WL6` | `49ba24e0b3916732cd065122de4fe6fb6e6a5009c353eafa407c0e3a5a503407` |

## Spear of Destiny demo (`SDM`)

| File | SHA-256 |
| --- | --- |
| `AUDIOHED.SDM` | `47f3236be1544f51d1dd993a168eeaa22da672fdfa30fb6a57947167afec7d25` |
| `AUDIOT.SDM` | `410cfca99bd4b17a229031134c5436fce2daeeed3708fc2d13aad3dc05d26d36` |
| `GAMEMAPS.SDM` | `93d6ae6ce7c9b9cbadee12003877f206432a197702dd63ef0fa8e27dafed2d77` |
| `MAPHEAD.SDM` | `4df9b8f9999a7542ba1172ba91c153e42b345bd4fe3466f659c5613451d2ec87` |
| `VGADICT.SDM` | `a80aa6ad4052114209db3961c81864a692172d9d86db4af1901e4b5178aa8ef3` |
| `VGAGRAPH.SDM` | `718d69c4ff992b37bd1647cf7c16111023e69674a06e999d12847d0536ec434b` |
| `VGAHEAD.SDM` | `776252326c47d82356d170ee7f03b8e96edbbc059fdd8167f21a21d2e8aa5148` |
| `VSWAP.SDM` | `2312bcadc05bbcbfce24514ab3000a15a2979fa62db9075271b762823588074e` |

This set contains two maps, 666 VSWAP pages, 125 graphics pictures, and the
original two-part demo title. It is exercised independently from full SOD.

## Spear of Destiny and mission packs (GOG layout)

The tested GOG installation places each mission in its own `M1`, `M2`, or
`M3` directory and uses `.SOD` for every archive. Select `--game SOD` for M1,
`--game SD2` for M2, and `--game SD3` for M3. The historically named
`.SD1`/`.SD2`/`.SD3` map/page layout is supported as well.

All three directories share these files:

| File | SHA-256 |
| --- | --- |
| `AUDIOHED.SOD` | `74f038a0d17e3075a8ed8be58b58a6f4ce590cd5371a140be49f8312de0415a5` |
| `AUDIOT.SOD` | `531b33871d4503f6f5e9131225d0739a6a24b07b0d02de62723bba813719e909` |
| `VGADICT.SOD` | `80713ce71576626acf5f83701ae163bc15511ce6a22416748d47d80bf406ce3d` |
| `VGAGRAPH.SOD` | `80f96fbaf7fa91c1eb5a8aa1d4bffdebce8a9b508aa60a1354bdcc50fa537d97` |
| `VGAHEAD.SOD` | `79950407f8948fa09479d8ba738aaeadc146e115a9d23e289e1a2105361e5909` |

Mission-specific archives:

| Mission | File | SHA-256 |
| --- | --- | --- |
| M1 / Spear of Destiny | `GAMEMAPS.SOD` | `772d834d97d429388be3cd7caa517e49a7e82e06367bd6f16d46c6b96e25ae1d` |
| M1 / Spear of Destiny | `MAPHEAD.SOD` | `3093aa7b0c88a3dfac9f5cc16d9f1b2fee338a9c4f43e1063ce15ca109498736` |
| M1 / Spear of Destiny | `VSWAP.SOD` | `6d9e54808a4738f11c37964d032c9a1c68a8ca98fcb18e6ccba286502967a568` |
| M2 / Return to Danger | `GAMEMAPS.SOD` | `29c6c1c3dbd2fe21e2e642615a0f6943952c1d7aff70e148d1b3b54e254899ba` |
| M2 / Return to Danger | `MAPHEAD.SOD` | `b3749cd2175284ed8b3ae4dbbd3704f5ef116b30ea49a373fed5538170bc0bbe` |
| M2 / Return to Danger | `VSWAP.SOD` | `97112755b1a07ab9ef5031f76ff04288de7e5bf910020e965f4bde9bcb7a96a6` |
| M3 / Ultimate Challenge | `GAMEMAPS.SOD` | `4f4a9d0a9cda58eb6dfc6663b8e63090a7f4e941b71b01261964c242b9bc7470` |
| M3 / Ultimate Challenge | `MAPHEAD.SOD` | `3d7f82e2d578f9081da6887119c6324e19137dae5bee11cdf6a8e8147fc5d503` |
| M3 / Ultimate Challenge | `VSWAP.SOD` | `d3e357682f5ff5e54c7406a191d207e8e25bfd2d42c80206a2e378fb658c4c5d` |

Other revisions are rejected when their archive structure does not match the
supported editions. Executables, configuration files, and save games are not
input assets and are intentionally absent from this table.
