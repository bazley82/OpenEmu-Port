/*
    This file is part of CrabEmu.

    Copyright (C) 2015 Lawrence Sebald

    CrabEmu is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    CrabEmu is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CrabEmu; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef INSIDE_CHIP8CPU_EXECUTE
#error This file can only be compiled inside of chip8cpu.c. Do not try to
#error include this file in other files.
#endif

uint8 _tmp, _tmp2;
int _tmp3;

switch(inst >> 12) {
    case 0x0:
        switch(inst) {
            case 0xE0:                  /* CLS */
                cpu->clear_fb(cpu);
                goto out;

            case 0xEE:                  /* RET */
                --cpu->sp;
                cpu->pc = cpu->stack[cpu->sp & 0xF];
                goto out;

            default:
                /* XXXX */
                goto out;
        }

        break;

    case 0x1:                           /* JP #addr12 */
        cpu->pc = inst & 0x0FFF;
        goto out;

    case 0x2:                           /* CALL #addr12 */
        cpu->stack[cpu->sp & 0xF] = cpu->pc;
        ++cpu->sp;
        cpu->pc = inst & 0x0FFF;
        goto out;

    case 0x3:                           /* SE Vx, #imm8 */
        _tmp = cpu->v[(inst >> 8) & 0x0F];
        _tmp2 = (uint8)inst;

        if(_tmp == _tmp2)
            cpu->pc += 2;
        goto out;

    case 0x4:                           /* SNE VX, #imm8 */
        _tmp = cpu->v[(inst >> 8) & 0x0F];
        _tmp2 = (uint8)inst;

        if(_tmp != _tmp2)
            cpu->pc += 2;
        goto out;

    case 0x5:                           /* SE Vx, Vy */
        /* XXXX: What if the last nibble isn't 0? */
        _tmp = cpu->v[(inst >> 8) & 0x0F];
        _tmp2 = cpu->v[(inst >> 4) & 0x0F];

        if(_tmp == _tmp2)
            cpu->pc += 2;
        goto out;

    case 0x6:                           /* LD Vx, #imm8 */
        cpu->v[(inst >> 8) & 0x0F] = (uint8)inst;
        goto out;

    case 0x7:                           /* ADD Vx, #imm8 */
        cpu->v[(inst >> 8) & 0x0F] += (uint8)inst;
        goto out;

    case 0x8:
        switch(inst & 0x000F) {
            case 0x0:                   /* LD Vx, Vy */
                cpu->v[(inst >> 8) & 0x0F] = cpu->v[(inst >> 4) & 0x0F];
                goto out;

            case 0x1:                   /* OR Vx, Vy */
                cpu->v[(inst >> 8) & 0x0F] |= cpu->v[(inst >> 4) & 0x0F];
                goto out;

            case 0x2:                   /* AND Vx, Vy */
                cpu->v[(inst >> 8) & 0x0F] &= cpu->v[(inst >> 4) & 0x0F];
                goto out;

            case 0x3:                   /* XOR Vx, Vy */
                cpu->v[(inst >> 8) & 0x0F] ^= cpu->v[(inst >> 4) & 0x0F];
                goto out;

            case 0x4:                   /* ADD Vx, Vy */
                _tmp = (inst >> 8) & 0x0F;
                _tmp3 = cpu->v[_tmp] + cpu->v[(inst >> 4) & 0x0F];
                cpu->v[_tmp] = (uint8)_tmp3;
                cpu->v[0xF] = (uint8)(_tmp3 >> 8) & 0x01;
                goto out;

            case 0x5:                   /* SUB Vx, Vy */
                _tmp = (inst >> 8) & 0xF;
                _tmp2 = (inst >> 4) & 0xF;
                _tmp3 = cpu->v[_tmp] - cpu->v[_tmp2];
                cpu->v[0xF] = !!(cpu->v[_tmp] >= cpu->v[_tmp2]);
                cpu->v[_tmp] = (uint8)_tmp3;
                goto out;

            case 0x6:                   /* SHR Vx {, Vy} */
                _tmp = (inst >> 8) & 0x0F;
                cpu->v[0xF] = cpu->v[_tmp] & 0x01;
                cpu->v[_tmp] >>= 1;
                goto out;

            case 0x7:                   /* SUBN Vx, Vy */
                _tmp = (inst >> 8) & 0xF;
                _tmp2 = (inst >> 4) & 0xF;
                _tmp3 = cpu->v[_tmp2] - cpu->v[_tmp];
                cpu->v[0xF] = !!(cpu->v[_tmp2] >= cpu->v[_tmp]);
                cpu->v[_tmp] = (uint8)_tmp3;
                goto out;

            case 0xE:                   /* SHL Vx {, Vy} */
                _tmp = (inst >> 8) & 0x0F;
                cpu->v[0xF] = (cpu->v[_tmp] >> 7) & 0x01;
                cpu->v[_tmp] <<= 1;
                goto out;

            default:
                /* XXXX */
                goto out;
        }

    case 0x9:                           /* SNE Vx, Vy */
        /* XXXX: What if the last nibble isn't 0? */
        _tmp = cpu->v[(inst >> 8) & 0x0F];
        _tmp2 = cpu->v[(inst >> 4) & 0x0F];

        if(_tmp != _tmp2)
            cpu->pc += 2;
        goto out;

    case 0xA:                           /* LD I, #addr12 */
        cpu->i = inst & 0x0FFF;
        goto out;

    case 0xB:                           /* JP V0, #addr12 */
        cpu->pc = (inst & 0x0FFF) + cpu->v[0];
        goto out;

    case 0xC:                           /* RND Vx, #imm8 */
        _tmp = (uint8)inst;
        cpu->v[(inst >> 8) & 0xF] = rand() & _tmp;
        goto out;

    case 0xD:                           /* DRW Vx, Vy, #imm4 */
        cpu->v[0xF] = cpu->draw_spr(cpu, cpu->i, cpu->v[(inst >> 8) & 0xF],
                                    cpu->v[(inst >> 4) & 0xF], inst & 0xF);
        goto out;

    case 0xE:
        switch(inst & 0xFF) {
            case 0x9E:                  /* SKP Vx */
                _tmp = cpu->v[(inst >> 8) & 0xF] & 0xF;
                if(cpu->rread(cpu, _tmp))
                    cpu->pc += 2;
                goto out;

            case 0xA1:                  /* SKNP Vx */
                _tmp = cpu->v[(inst >> 8) & 0xF] & 0xF;
                if(!cpu->rread(cpu, _tmp))
                    cpu->pc += 2;
                goto out;

            default:
                /* XXXX */
                goto out;
        }

    case 0xF:
        switch(inst & 0xFF) {
            case 0x07:                  /* LD Vx, DT */
                cpu->v[(inst >> 8) & 0xF] = cpu->rread(cpu, CHIP8_REG_DT);
                goto out;

            case 0x0A:                  /* LD Vx, K */
                for(_tmp = 0; _tmp < 15; ++_tmp) {
                    if(cpu->rread(cpu, _tmp)) {
                        cpu->v[(inst >> 8) & 0xF] = _tmp;
                        goto out;
                    }
                }

                /* Didn't find a key pressed, so repeat the instruction. */
                cpu->pc -= 2;
                goto out;

            case 0x15:                  /* LD DT, Vx */
                cpu->rwrite(cpu, CHIP8_REG_DT, cpu->v[(inst >> 8) & 0xF]);
                goto out;

            case 0x18:                  /* LD ST, Vx */
                cpu->rwrite(cpu, CHIP8_REG_ST, cpu->v[(inst >> 8) & 0xF]);
                goto out;

            case 0x1E:                  /* ADD I, Vx */
                cpu->i += cpu->v[(inst >> 8) & 0xF];
                cpu->v[0xF] = !!(cpu->i >> 12);
                goto out;

            case 0x29:                  /* LD F, Vx */
                /* The font is at 0x000, so this is easy. */
                cpu->i = (cpu->v[(inst >> 8) & 0xF] & 0xF) * 5;
                goto out;

            case 0x33:                  /* LD B, Vx */
                _tmp = cpu->v[(inst >> 8) & 0xF];
                cpu->mwrite(cpu, cpu->i, _tmp / 100);
                cpu->mwrite(cpu, cpu->i + 1, (_tmp % 100) / 10);
                cpu->mwrite(cpu, cpu->i + 2, _tmp % 10);
                goto out;

            case 0x55:                  /* LD [I], Vx */
                _tmp2 = (inst >> 8) & 0xF;

                for(_tmp = 0; _tmp <= _tmp2; ++_tmp) {
                    cpu->mwrite(cpu, cpu->i + _tmp, cpu->v[_tmp]);
                }

                cpu->i += _tmp2 + 1;

                goto out;

            case 0x65:                  /* LD Vx, [I] */
                _tmp2 = (inst >> 8) & 0xF;

                for(_tmp = 0; _tmp <= _tmp2; ++_tmp) {
                    cpu->v[_tmp] = cpu->mread(cpu, cpu->i + _tmp);
                }

                cpu->i += _tmp2 + 1;

                goto out;

            default:
                /* XXXX */
                goto out;
        }
}

out:
    ;
