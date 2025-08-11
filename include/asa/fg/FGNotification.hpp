#ifndef __asa_fg_notification_H__
#define __asa_fg_notification_H__

#include <cstddef>
#include <string>

namespace asa::fg
{
    class FGNotification
    {
    public:
        FGNotification(int mpidx) : mpidx(mpidx) {}
        virtual size_t bufSize() const = 0;
        virtual std::string encode() = 0;
        static bool decode(std::string);
        virtual ~FGNotification() {}

        int getMpidx() const { return mpidx; }
        void setMpidx(int mpidx_) { mpidx = mpidx_; }

    protected:
        int mpidx;
    };

}
#endif /* __asa_fg_notification_H__ */