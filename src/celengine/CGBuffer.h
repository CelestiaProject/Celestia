/*

Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
                ("Apple") in consideration of your agreement to the following terms, and your
                use, installation, modification or redistribution of this Apple software
                constitutes acceptance of these terms.  If you do not agree with these terms,
                please do not use, install, modify or redistribute this Apple software.

                In consideration of your agreement to abide by the following terms, and subject
                to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
                copyrights in this original Apple software (the "Apple Software"), to use,
                reproduce, modify and redistribute the Apple Software, with or without
                modifications, in source and/or binary forms; provided that if you redistribute
                the Apple Software in its entirety and without modifications, you must retain
                this notice and the following text and disclaimers in all such redistributions of
                the Apple Software.  Neither the name, trademarks, service marks or logos of
                Apple Computer, Inc. may be used to endorse or promote products derived from the
                Apple Software without specific prior written permission from Apple.  Except as
                expressly stated in this notice, no other rights or licenses, express or implied,
                are granted by Apple herein, including but not limited to any patent rights that
                may be infringed by your derivative works or by other works in which the Apple
                Software may be incorporated.

                The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
                WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
                WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
                PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
                COMBINATION WITH YOUR PRODUCTS.

                IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
                CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
                GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
                ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
                OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
                (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
                ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// Note that this is modified Apple source code, and not in its entireity (some classes are, some aren't), 
// but we might as well leave it in here, even though it doesn't sound like we have to.
// -bob

#include <mach/mach.h>
#include <Carbon/Carbon.h>
// Celestia's DPRINTF is different than Carbon's
#undef DPRINTF
#include <celutil/debug.h>

class CGFrame : public CGRect
{
public:

	CGFrame(float x0, float y0, float w, float h)
	{
		*this = CGRectMake(x0,y0,w,h);
	}
	
	explicit CGFrame(float w = 0, float h = 0)
	{
		*this = CGRectMake(0,0,w,h);
	}
	
	CGFrame(const Rect& rect)
	{
		origin.x = rect.left, size.width = rect.right - rect.left;
		origin.y = rect.top, size.height = rect.bottom - rect.top;
	}
	
	CGFrame(const CGRect& copy)
	{
		origin = copy.origin;
		size = copy.size;
	}
	
	CGFrame(const CGSize& size)
	{
		origin.x = origin.y = 0;
		this->size = size;
	}
	
	CGFrame(float x, float y, const CGSize& size)
	{
		*this = CGRectMake(x, y, size.width, size.height);
	}
	
	CGFrame(const CGPoint& pos, const CGSize& size)
	{
		*this = CGRectMake(pos.x, pos.y, size.width, size.height);
	}	
	
	void Offset(float dx, float dy)
	{
		origin.x += dx, origin.y += dy;
	}
	
	void Inset(float dx, float dy)
	{
		origin.x += dx, origin.y += dy;
		size.width -= dx*2, size.height -= dy*2;
	}
};

class MemoryBuffer
{
  size_t size;
  int ref_count;

  MemoryBuffer(char* const data, size_t size) : size(size), ref_count(1), data(data)
  {
  }

  ~MemoryBuffer()
  {
    vm_deallocate((vm_map_t) mach_task_self(), (vm_address_t) data, size);
  }

public:

  friend class _MemoryBuffer; // to shut the compiler up

  char* const data;

  MemoryBuffer* Retain()
  {
    ++ref_count;
    return this;
  }

  void Release()
  {
    if (--ref_count == 0)
      delete this;
  }

  static MemoryBuffer* Create(size_t size)
  {
    char* data;
    kern_return_t err = vm_allocate((vm_map_t) mach_task_self(), (vm_address_t*) &data, size, TRUE);

    return (err == KERN_SUCCESS) ? new MemoryBuffer(data, size) : NULL;
  }
};

class Datafile
{
    int ref_count;
    FILE* file;
    
public:        
    MemoryBuffer* data_buffer;
    unsigned long data_size;

    Datafile() : ref_count(1), file(NULL), data_buffer(NULL), data_size(0)
    {
    }

    ~Datafile()
    {
        Reset();
    }

    Datafile* Retain()
    {
        ++ref_count;
        return this;
    }
    
    void Release()
    {
        if (--ref_count==0) 
            delete this;
    }
    
    int Open(const char* path)
    {
        file = fopen(path,"r");
        if (!file) {
            DPRINTF(0,"Datafile::Open() - Couldn't open %s\n", path);
            Reset();
            return 0;
        }
        fseek(file, 0, SEEK_END);
        data_size = ftell(file);
        data_buffer = MemoryBuffer::Create(data_size);
        if (!data_buffer) {
            DPRINTF(0,"Datafile::Open() - Couldn't allocate MemoryBuffer of size %d\n", data_size);
            Reset();
            return 0;
        }
        //DPRINTF(0,"Datafile::Open() - Successfully opened '%s' %d bytes\n", path, data_size);
        return 1;
    }
    
    int Read()
    {
        if ((file == NULL) || (data_buffer == NULL) || (data_size == 0)) {
            DPRINTF(0,"Datafile::Read() - No file open, file of zero size, or no valid MemoryBuffer\n");
            Reset();
            return 0;
        }
        
        fseek(file, 0, SEEK_SET);        
        if (fread((void*)data_buffer->data, 1, data_size, file) != data_size) {
            DPRINTF(0,"Datafile::Read() - Didn't read to finish?");
            Reset();
            return 0;
        }
        
        //DPRINTF(0,"Datafile::Read() - Successfully read all %d bytes into buffer\n",data_size);
        return 1;
    }
    
    void Close()
    {
        if (file) {
            fclose(file);
            file = NULL;
        }
    }
    
    void Reset()
    {
        Close();
        if (data_buffer) {
            data_buffer->Release();
            data_buffer = NULL;
        }
    }
};

class CGBuffer
{
	Datafile file;
	CGImageRef image_ref;
	CGContextRef context_ref;

	int ref_count;

	void Init()
	{
		ref_count = 1;
		buffer = NULL;
		image_ref = NULL;
		context_ref = NULL;
		image_finished = false;
	}

	bool CreateCGContext()
	{
		if (context_ref)
		{
			CGContextRelease(context_ref);
			context_ref = NULL;
		}

		if (buffer)
		{
			buffer->Release();
			buffer = NULL;
		}

		size_t buffer_rowbytes = (size_t)(image_size.width * ((image_depth == 8) ? 1 : 4)); //CGImageGetBytesPerRow(image_ref);
	
		buffer = MemoryBuffer::Create(buffer_rowbytes * (size_t)image_size.height); 

		if (!buffer)
			return false;

		CGColorSpaceRef colorspace_ref = (image_depth == 8) ? CGColorSpaceCreateDeviceGray() : CGColorSpaceCreateDeviceRGB();
		
		if (!colorspace_ref)
			return false;
		
		CGImageAlphaInfo alpha_info = (image_depth == 8) ? kCGImageAlphaNone : kCGImageAlphaPremultipliedLast; //kCGImageAlphaLast; //RGBA format

		context_ref = CGBitmapContextCreate(buffer->data, (size_t)image_size.width, (size_t)image_size.height, 8, buffer_rowbytes, colorspace_ref, alpha_info);

		if (context_ref)
		{
			CGContextSetFillColorSpace(context_ref, colorspace_ref);
			CGContextSetStrokeColorSpace(context_ref, colorspace_ref);
                        // move down, and flip vertically 
                        // to turn postscript style coordinates to "screen style"
			CGContextTranslateCTM(context_ref, 0.0, image_size.height);
			CGContextScaleCTM(context_ref, 1.0, -1.0);
		}
		
		CGColorSpaceRelease(colorspace_ref);
		colorspace_ref = NULL;

		return !!context_ref;
	}

	void RenderCGImage(const CGRect& dst_rect)
	{
		if (!context_ref || !image_ref)
			return;
			
		CGContextDrawImage(context_ref, dst_rect, image_ref);
	}
	
public:

	MemoryBuffer* buffer;
	CGSize image_size;
	size_t image_depth;
	bool image_finished;
	
	CGBuffer(const char* path)
	{
		Init();
		Open(path);
	}

        ~CGBuffer()
	{
		Reset();
	}

	CGBuffer* Retain()
	{
		++ref_count;
		return this;
	}

	void Release()
	{
		if (--ref_count == 0)
			delete this;
	}

	bool Open(const char* path)
	{
		file.Reset();
		return file.Open(path);
	}

	bool LoadJPEG()
	{
		if (!file.Read())
			return false;

		file.Close();

		CGDataProviderRef src_provider_ref = CGDataProviderCreateWithData(this, file.data_buffer->data, file.data_size, NULL);

		if (!src_provider_ref)
			return false;

		image_ref = CGImageCreateWithJPEGDataProvider(src_provider_ref, NULL, true, kCGRenderingIntentDefault);

		CGDataProviderRelease(src_provider_ref);
		src_provider_ref = NULL;

		if (!image_ref)
			return false;

		image_size = CGSizeMake(CGImageGetWidth(image_ref), CGImageGetHeight(image_ref));
		image_depth = CGImageGetBitsPerPixel(image_ref);
		
		return !!image_ref;
	}

	bool Render()
	{
		if (!image_ref)
			return false;
		
		if (!CreateCGContext())
			return false;
	
		RenderCGImage(CGFrame(image_size));
		
		CGContextRelease(context_ref);
		context_ref = NULL;

		CGImageRelease(image_ref);
		image_ref = NULL;
		
		file.Reset();

		return true;
	}
	
	void Reset()
	{
		if (buffer)
		{
			buffer->Release();
			buffer = NULL;
		}

		if (image_ref)
		{
			CGImageRelease(image_ref);
			image_ref = NULL;
		}
		
		if (context_ref)
		{
			CGContextRelease(context_ref);
			context_ref = NULL;
		}
	}
};