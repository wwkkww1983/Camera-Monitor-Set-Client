/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2MmapDevice.cpp
** 
** V4L2 source using mmap API
**
** -------------------------------------------------------------------------*/

#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h> 
#include <sys/mman.h>
#include <sys/ioctl.h>

// libv4l2
#include <linux/videodev2.h>

// project
#include "logger.h"
#include "V4l2MmapDevice.h"

V4l2MmapDevice::V4l2MmapDevice(const V4L2DeviceParameters & params, v4l2_buf_type deviceType) : V4l2Device(params, deviceType), n_buffers(0) 
{
	memset(&m_buffer, 0, sizeof(m_buffer));
}

bool V4l2MmapDevice::init(unsigned int mandatoryCapabilities)
{
	bool ret = V4l2Device::init(mandatoryCapabilities);
	if (ret)
	{
		ret = this->start();
	}
	return ret;
}

V4l2MmapDevice::~V4l2MmapDevice()
{
	this->stop();
}


bool V4l2MmapDevice::start() 
{
	LOG(NOTICE) << "Device " << m_params.m_devName;

	bool success = true;
	struct v4l2_requestbuffers req;
	memset (&req, 0, sizeof(req));
	req.count               = V4L2MMAP_NBBUFFER;
	req.type                = m_deviceType;
	req.memory              = V4L2_MEMORY_MMAP;

	if (-1 == ioctl(m_fd, VIDIOC_REQBUFS, &req)) 
	{
		if (EINVAL == errno) 
		{
			LOG(ERROR) << "Device " << m_params.m_devName << " does not support memory mapping";
			success = false;
		} 
		else 
		{
			perror("VIDIOC_REQBUFS");
			success = false;
		}
	}
	else
	{
		LOG(NOTICE) << "Device " << m_params.m_devName << " nb buffer:" << req.count;
		
		// allocate buffers
		memset(&m_buffer,0, sizeof(m_buffer));
		for (n_buffers = 0; n_buffers < req.count; ++n_buffers) 
		{
			struct v4l2_buffer buf;
			memset (&buf, 0, sizeof(buf));
			buf.type        = m_deviceType;
			buf.memory      = V4L2_MEMORY_MMAP;
			buf.index       = n_buffers;

			if (-1 == ioctl(m_fd, VIDIOC_QUERYBUF, &buf))
			{
				perror("VIDIOC_QUERYBUF");
				success = false;
			}
			else
			{
				LOG(INFO) << "Device " << m_params.m_devName << " buffer idx:" << n_buffers << " size:" << buf.length << " offset:" << buf.m.offset;
				m_buffer[n_buffers].length = buf.length;
				if (!m_buffer[n_buffers].length) {
					m_buffer[n_buffers].length = buf.bytesused;
				}
				m_buffer[n_buffers].start = mmap (   NULL /* start anywhere */, 
											m_buffer[n_buffers].length, 
											PROT_READ | PROT_WRITE /* required */, 
											MAP_SHARED /* recommended */, 
											m_fd, 
											buf.m.offset);

				if (MAP_FAILED == m_buffer[n_buffers].start)
				{
					perror("mmap");
					success = false;
				}
			}
		}

		// queue buffers
		for (unsigned int i = 0; i < n_buffers; ++i) 
		{
			struct v4l2_buffer buf;
			memset (&buf, 0, sizeof(buf));
			buf.type        = m_deviceType;
			buf.memory      = V4L2_MEMORY_MMAP;
			buf.index       = i;

			if (-1 == ioctl(m_fd, VIDIOC_QBUF, &buf))
			{
				perror("VIDIOC_QBUF");
				success = false;
			}
		}

		// start stream
		int type = m_deviceType;
		if (-1 == ioctl(m_fd, VIDIOC_STREAMON, &type))
		{
			perror("VIDIOC_STREAMON");
			success = false;
		}
	}
	return success; 
}

bool V4l2MmapDevice::stop() 
{
	LOG(NOTICE) << "Device " << m_params.m_devName;

	bool success = true;
	
	int type = m_deviceType;
	if (-1 == ioctl(m_fd, VIDIOC_STREAMOFF, &type))
	{
		perror("VIDIOC_STREAMOFF");      
		success = false;
	}

	for (unsigned int i = 0; i < n_buffers; ++i)
	{
		if (-1 == munmap (m_buffer[i].start, m_buffer[i].length))
		{
			perror("munmap");
			success = false;
		}
	}
	
	// free buffers
	struct v4l2_requestbuffers req;
	memset (&req, 0, sizeof(req));
	req.count               = 0;
	req.type                = m_deviceType;
	req.memory              = V4L2_MEMORY_MMAP;
	if (-1 == ioctl(m_fd, VIDIOC_REQBUFS, &req)) 
	{
		perror("VIDIOC_REQBUFS");
		success = false;
	}
	
	n_buffers = 0;
	return success; 
}

size_t V4l2MmapDevice::readInternal(char* buffer, size_t bufferSize)
{
	size_t size = 0;
	if (n_buffers > 0)
	{
		struct v4l2_buffer buf;	
		memset (&buf, 0, sizeof(buf));
		buf.type = m_deviceType;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == ioctl(m_fd, VIDIOC_DQBUF, &buf)) 
		{
			perror("VIDIOC_DQBUF");
			size = -1;
		}
		else if (buf.index < n_buffers)
		{
			size = buf.bytesused;
			if (size > bufferSize)
			{
				size = bufferSize;
				LOG(WARN) << "Device " << m_params.m_devName << " buffer truncated available:" << bufferSize << " needed:" << buf.bytesused;
			}
			memcpy(buffer, m_buffer[buf.index].start, size);

			if (-1 == ioctl(m_fd, VIDIOC_QBUF, &buf))
			{
				perror("VIDIOC_QBUF");
				size = -1;
			}
		}
	}
	return size;
}

size_t V4l2MmapDevice::writeInternal(char* buffer, size_t bufferSize)
{
	size_t size = 0;
	if (n_buffers > 0)
	{
		struct v4l2_buffer buf;	
		memset (&buf, 0, sizeof(buf));
		buf.type = m_deviceType;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == ioctl(m_fd, VIDIOC_DQBUF, &buf)) 
		{
			perror("VIDIOC_DQBUF");
			size = -1;
		}
		else if (buf.index < n_buffers)
		{
			size = bufferSize;
			if (size > buf.length)
			{
				size = buf.length;
				LOG(WARN) << "Device " << m_params.m_devName << " buffer truncated available:" << buf.length << " needed:" << size;
			}
			memcpy(m_buffer[buf.index].start, buffer, size);
			buf.bytesused = size;

			if (-1 == ioctl(m_fd, VIDIOC_QBUF, &buf))
			{
				perror("VIDIOC_QBUF");
				size = -1;
			}
		}
	}
	return size;
}




