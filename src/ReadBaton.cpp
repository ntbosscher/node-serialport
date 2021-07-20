
#include "./ReadBaton.h"
#include "./V8ArgDecoder.h"

v8::Local<v8::Value> ReadBaton::getReturnValue()
{
    return Nan::New<v8::Integer>(static_cast<int>(bytesRead));
}

NAN_METHOD(Read)
{
        V8ArgDecoder args(&info);

        auto fd = args.takeInt32();
        auto buffer = args.takeBuffer();
        auto offset = args.takeInt32();
        auto bytesToRead = args.takeInt32();
        auto timeout = args.takeInt32();
        auto cb = args.takeFunction();

        if(args.hasError()) return;

        if ((bytesToRead + offset) > buffer.length)
        {
            Nan::ThrowTypeError("'bytesToRead' + 'offset' cannot be larger than the buffer's length");
            return;
        }

        ReadBaton *baton = new ReadBaton("read-baton", cb);

        baton->fd = fd;
        baton->offset = offset;
        baton->bytesToRead = bytesToRead;
        baton->bufferLength = buffer.length;
        baton->bufferData = buffer.buffer;
        baton->timeout = timeout;
        baton->complete = false;

        baton->start();
}

void ReadBaton::run()
{

    int start = currentMs();
    int deadline = start + this->timeout;

    if(verbose) {
        muLogger.lock();
        auto out = defaultLogger();
        out << currentMs() << " " << debugName << " expecting=" << bytesToRead << " have=" << bytesRead << " blocking=" << true << " \n";
        out.close();
        muLogger.unlock();
    }

    do
    {
        char *buffer = bufferData + offset;

        auto bytesTransferred = readFromSerial(fd, buffer, bytesToRead, true, errorString);
        if(bytesTransferred < 0) {
            complete = true;
            return; // error
        }

        bytesToRead -= bytesTransferred;
        bytesRead += bytesTransferred;
        offset += bytesTransferred;
        complete = bytesToRead == 0;

        if(complete && verbose) {
            muLogger.lock();
            auto out = defaultLogger();
            out << currentMs() << " " << debugName << " got " << bytesTransferred << " bytes, buffer-contents: " << bufferToHex(bufferData, bytesRead);
            out << "\n";
            out.close();
            muLogger.unlock();
        }

    } while (!complete && currentMs() < deadline);

    if(!complete) {
        muLogger.lock();
        auto out = defaultLogger();
        out << currentMs() << " " << debugName << " got " << bytesRead << " bytes, buffer-contents: " << bufferToHex(bufferData, bytesRead);
        out << "\n";
        out.close();
        muLogger.unlock();
    }
}