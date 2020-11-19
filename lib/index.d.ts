export default class FasterSerialPort {
    isOpen: boolean;
  
    constructor(path: string, options: any);
  
    write(buf: Buffer | number[]): Promise<void>;
    read(nBytes: number): Promise<Buffer>;
    setTimeout(ms: number): void;
    close(): Promise<void>;
    open(): Promise<void>;
    update(opts: {
      baudRate:
        | 115200
        | 57600
        | 38400
        | 19200
        | 9600
        | 4800
        | 2400
        | 1800
        | 1200
        | 600
        | 300
        | 200
        | 150
        | 134
        | 110
        | 75
        | 50
        | number;
    }): Promise<void>;
  
    static list(): Promise<PortInfo[]>;
  }

  export interface PortInfo {
    path: string;
    manufacturer?: string;
    serialNumber?: string;
    pnpId?: string;
    locationId?: string;
    productId?: string;
    vendorId?: string;
  }