
#include "Intellivision.h"

/**
 * Initializes all of the basic hardware included in the Intellivision
 * Master Component as well as the ECS and Intellivoice peripherals.
 * This method is called only once when the Intellivision is constructed.
 */
Intellivision::Intellivision()
    : Emulator("Intellivision"),
      player1Controller(0, "Hand Controller 1"),
      player2Controller(1, "Hand Controller 2"),
      psg(0x01F0, &player1Controller, &player2Controller),
      RAM8bit(RAM8BIT_SIZE, 0x0100, 8),
      RAM16bit(RAM16BIT_SIZE, 0x0200, 16),
      execROM("Executive ROM", "exec.bin", 0, 2, 0x1000, 0x1000),
      grom("GROM", "grom.bin", 0, 1, 0x0800, 0x3000),
      gram(),
      cpu(&memoryBus, 0x1000, 0x1004),
      stic(&memoryBus, &grom, &gram)
{
    // define the video pixel dimensions
    videoWidth = 160;
    videoHeight = 192;

    //make the pin connections from the CPU to the STIC
    stic.connectPinOut(AY38900_PIN_OUT_SR1, &cpu, CP1610_PIN_IN_INTRM);
    stic.connectPinOut(AY38900_PIN_OUT_SR2, &cpu, CP1610_PIN_IN_BUSRQ);
    cpu.connectPinOut(CP1610_PIN_OUT_BUSAK, &stic, AY38900_PIN_IN_SST);

    //add the player one hand controller
    AddInputConsumer(&player1Controller);

    //add the player two hand controller
    AddInputConsumer(&player2Controller);

    //add the 8-bit RAM
    AddRAM(&RAM8bit);

    //add the 16-bit RAM
    AddRAM(&RAM16bit);

    //add the executive ROM
    AddROM(&execROM);

    //add the GROM
    AddROM(&grom);

    //add the GRAM
    AddRAM(&gram);

    //add the backtab ram
    AddRAM(&stic.backtab);

    //add the CPU
    AddProcessor(&cpu);

    //add the STIC
    AddProcessor(&stic);
    AddVideoProducer(&stic);

    //add the STIC registers
    AddRAM(&stic.registers);

    //add the PSG
    AddProcessor(&psg);
    AddAudioProducer(&psg);

    //add the PSG registers
    AddRAM(&psg.registers);

    AddPeripheral(&ecs);
    AddPeripheral(&intellivoice);

    //zero the emulator state
    memset(&state, 0, sizeof(IntellivisionState));
}

void Intellivision::SaveState()
{
    state.header.emu = FOURCHAR('EMUS');
    state.header.state = FOURCHAR('TATE');
    state.header.emuID = ID_EMULATOR_BLISS;
    state.header.version = FOURCHAR(EMU_STATE_VERSION);
    state.header.sys = FOURCHAR('SYS\0');
    state.header.sysID = ID_SYSTEM_INTELLIVISION;
    state.header.cart = FOURCHAR('CART');
    state.header.cartID = currentRip->GetCRC();

    state.cpu.id = FOURCHAR('CPU\0');
    state.cpu.size = sizeof(CP1610State);
    state.cpuState = cpu.getState();

    state.stic.id = FOURCHAR('STIC');
    state.stic.size = sizeof(AY38900State);
    state.sticState = stic.getState();

    state.psg.id = FOURCHAR('PSG\0');
    state.psg.size = sizeof(AY38914State);
    state.psgState = psg.getState();

    state.RAM8bit.id = FOURCHAR('RAM0');
    state.RAM8bit.size = sizeof(RAMState) + sizeof(state.RAM8bitImage);
    state.RAM8bitState = RAM8bit.getState(state.RAM8bitImage);

    state.RAM16bit.id = FOURCHAR('RAM1');
    state.RAM16bit.size = sizeof(RAMState) + sizeof(state.RAM16bitImage);
    state.RAM16bitState = RAM16bit.getState(state.RAM16bitImage);

    state.GRAM.id = FOURCHAR('GRAM');
    state.GRAM.size = sizeof(RAMState) + sizeof(state.GRAMImage);
    state.GRAMState = gram.getState(state.GRAMImage);

    // TODO: only if ivoice is used for this cart?
    state.ivoice.id = FOURCHAR('VOIC');
    state.ivoice.size = sizeof(IntellivoiceState);
    state.ivoiceState = intellivoice.getState();

    // TODO: only if ecs is used for this cart?
    state.ecs.id = FOURCHAR('ECS\0');
    state.ecs.size = sizeof(ECSState);
    state.ecsState = ecs.getState();

    state.eof.id = FOURCHAR('EOF\0');
    state.eof.size = sizeof(IntellivisionState);
}

BOOL Intellivision::LoadState()
{
    if (!isStateValid(&state)) {
        return FALSE;
    }

    cpu.setState(state.cpuState);
    stic.setState(state.sticState);
    psg.setState(state.psgState);
    RAM8bit.setState(state.RAM8bitState, state.RAM8bitImage);
    RAM16bit.setState(state.RAM16bitState, state.RAM16bitImage);
    gram.setState(state.GRAMState, state.GRAMImage);
    intellivoice.setState(state.ivoiceState);
    ecs.setState(state.ecsState);

    return TRUE;
}

BOOL Intellivision::isStateValid(const IntellivisionState* state)
{
    if (state->header.emu != FOURCHAR('EMUS') || state->header.state != FOURCHAR('TATE')) {
        return FALSE;
    }

    if (state->header.emuID != ID_EMULATOR_BLISS) {
        return FALSE;
    }

    if (FOURCHAR(EMU_STATE_VERSION) != FOURCHAR('dev\0') && state->header.version != FOURCHAR('dev\0') && state->header.version != FOURCHAR(EMU_STATE_VERSION)) {
        return FALSE;
    }

    if (state->header.sys != FOURCHAR('SYS\0')) {
        return FALSE;
    }

    if (state->header.sysID != ID_SYSTEM_INTELLIVISION) {
        return FALSE;
    }

    if (state->header.cart != FOURCHAR('CART')) {
        return FALSE;
    }

    if (state->header.cartID != 0x00000000 && state->header.cartID != currentRip->GetCRC()) {
        return FALSE;
    }

    return TRUE;
}

BOOL Intellivision::SaveState(IntellivisionState* outState)
{
    SaveState();

    memcpy(outState, &state, sizeof(IntellivisionState));

    return TRUE;
}

BOOL Intellivision::LoadState(const IntellivisionState* inState)
{
    if (!isStateValid(inState)) {
        return FALSE;
    }

    memcpy(&state, inState, sizeof(IntellivisionState));

    return LoadState();
}

BOOL Intellivision::SaveStateBuffer(void* outBuffer, size_t bufferSize)
{
    IntellivisionState *bufferState = (IntellivisionState*)outBuffer;

    if(!outBuffer || bufferSize < sizeof(IntellivisionState)) {
        return FALSE;
    }

    return SaveState(bufferState);
}

BOOL Intellivision::LoadStateBuffer(const void* inBuffer, size_t bufferSize)
{
    IntellivisionState *bufferState = (IntellivisionState*)inBuffer;

    if(!inBuffer || bufferSize < sizeof(IntellivisionState)) {
        return FALSE;
    }

    return LoadState(bufferState);
}

BOOL Intellivision::SaveStateFile(const CHAR* filename)
{
    BOOL didSave = FALSE;
    size_t totalStateSize = sizeof(IntellivisionState);

    // save the current state internally
    SaveState();

    FILE* file = fopen(filename, "wb");

    if (file == NULL) {
        printf("Error: Unable to create file %s\n", filename);
        didSave = FALSE;
    }

    if (file != NULL && totalStateSize == fwrite(&state, 1, totalStateSize, file)) {
        didSave = TRUE;
    } else {
        printf("Error: could not write %zu bytes to file %s\n", totalStateSize, filename);
        didSave = FALSE;
    }

    if (file) {
        fclose(file);
        file = NULL;
    }

    return didSave;
}

BOOL Intellivision::LoadStateFile(const CHAR* filename)
{
    BOOL didLoadState = FALSE;
    IntellivisionState fileState = {0};
    size_t totalStateSize = sizeof(IntellivisionState);

    FILE* file = fopen(filename, "rb");

    if (file == NULL) {
        printf("Error: Unable to open file %s\n", filename);
        return FALSE;
    }
#if 0
    // read in the whole file
    if (totalStateSize != fread(&fileState, 1, totalStateSize, file)) {
        printf("Error: could not read state (%zu bytes) from file %s\n", totalStateSize, filename);
        goto close;
    }
#else
    BOOL isParsing = FALSE;
    StateChunk chunk = {0};

    // read in the header
    if (sizeof(StateHeader) != fread(&fileState, 1, sizeof(StateHeader), file)) {
        printf("Error: could not read state header (%zu bytes) from file %s\n", totalStateSize, filename);
        goto close;
    }

    // validate file header
    if (fileState.header.emu != FOURCHAR('EMUS') || fileState.header.state != FOURCHAR('TATE')) {
        printf("Error: invalid header in file %s\n", filename);
        goto close;
    }

    if (fileState.header.emuID != ID_EMULATOR_BLISS) {
        printf("Error: invalid emulator ID %x in file %s\n", fileState.header.emuID, filename);
        goto close;
    }

    if (FOURCHAR(EMU_STATE_VERSION) != FOURCHAR('dev\0') && fileState.header.version != FOURCHAR('dev\0') && fileState.header.version != FOURCHAR(EMU_STATE_VERSION)) {
        printf("Error: invalid emulator version 0x%08x (expected 0x%08x) in file %s\n", fileState.header.version, EMU_STATE_VERSION, filename);
        goto close;
    }

    if (fileState.header.sys != FOURCHAR('SYS\0')) {
        printf("Error: expected 'SYS ' chunk in file %s\n", filename);
        goto close;
    }

    if (fileState.header.sysID != ID_SYSTEM_INTELLIVISION) {
        printf("Error: invalid system ID %x in file %s\n", fileState.header.sysID, filename);
        goto close;
    }

    if (fileState.header.cart != FOURCHAR('CART')) {
        printf("Error: expected 'CART' chunk in file %s\n", filename);
        goto close;
    }

    if (fileState.header.cartID != 0x00000000 && fileState.header.cartID != currentRip->GetCRC()) {
        printf("Error: cartridge mismatch in file %s\n", filename);
        goto close;
    }

    isParsing = TRUE;
    while (isParsing) {
        size_t fpos = ftell(file);
        if (sizeof(StateChunk) != fread(&chunk, 1, sizeof(StateChunk), file)) {
            isParsing = FALSE;
            break;
        }

        switch (chunk.id) {
            default:
                fpos = ftell(file);
                break;
            case FOURCHAR('CPU\0'):
                if (chunk.size == sizeof(fileState.cpuState)) {
                    fileState.cpu = chunk;
                    fread(&fileState.cpuState, 1, fileState.cpu.size, file);
                }
                break;
            case FOURCHAR('STIC'):
                if (chunk.size == sizeof(fileState.sticState)) {
                    fileState.stic = chunk;
                    fread(&fileState.sticState, 1, fileState.stic.size, file);
                }
                break;
            case FOURCHAR('PSG\0'):
                if (chunk.size == sizeof(fileState.psgState)) {
                    fileState.psg = chunk;
                    fread(&fileState.psgState, 1, fileState.psg.size, file);
                }
                break;
            case FOURCHAR('RAM0'):
                if (chunk.size == sizeof(fileState.RAM8bitState) + sizeof(fileState.RAM8bitImage)) {
                    fileState.RAM8bit = chunk;
                    fread(&fileState.RAM8bitState, 1, fileState.RAM8bit.size, file);
                }
                break;
            case FOURCHAR('RAM1'):
                if (chunk.size == sizeof(fileState.RAM16bitState) + sizeof(fileState.RAM16bitImage)) {
                    fileState.RAM16bit = chunk;
                    fread(&fileState.RAM16bitState, 1, fileState.RAM16bit.size, file);
                }
                break;
            case FOURCHAR('GRAM'):
                if (chunk.size == sizeof(fileState.GRAMState) + sizeof(fileState.GRAMImage)) {
                    fileState.GRAM = chunk;
                    fread(&fileState.GRAMState, 1, fileState.GRAM.size, file);
                }
                break;
            case FOURCHAR('VOIC'):
                // TODO: only if ivoice/ecs is used for this cart?
                if (chunk.size == sizeof(fileState.ivoiceState)) {
                    fileState.ivoice = chunk;
                    fread(&fileState.ivoiceState, 1, fileState.ivoice.size, file);
                }
                break;
            case FOURCHAR('ECS\0'):
                // TODO: only if ivoice/ecs is used for this cart?
                if (chunk.size == sizeof(fileState.ecsState)) {
                    fileState.ecs = chunk;
                    fread(&fileState.ecsState, 1, fileState.ecs.size, file);
                }
                break;
            case FOURCHAR('EOF\0'):
                fileState.eof = chunk;
                isParsing = FALSE;
                break;
        }
    }
#endif
    didLoadState = TRUE;

close:
    fclose(file);
    file = NULL;
end:
    if (didLoadState) {
        return LoadState(&fileState);
    }

    return didLoadState;
}
