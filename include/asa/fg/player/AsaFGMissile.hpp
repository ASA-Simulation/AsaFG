#ifndef __asa_AsaFGMissile_H__
#define __asa_AsaFGMissile_H__

namespace asa::fg
{

    //------------------------------------------------------------------------------
    // Class: AsaFGAbstractMissile
    // Factory name: AsaFGAbstractMissile
    // Description:
    //------------------------------------------------------------------------------
    class AsaFGAbstractMissile : public asa::models::AsaAbstractMissile
    {
        DECLARE_SUBCLASS(AsaFGAbstractMissile, asa::models::AsaAbstractMissile)

    public:
        AsaFGAbstractMissile();

        void dynamics(const double dt) override;

    protected:
        double seekerOnDist{}; // [m]
        double seekerGimbal{}; // [deg]

    private:
        bool seekerOn{};
    };
    //------------------------------------------------------------------------------
    // Class: AsaFGMissile
    // Factory name: AsaFGMissile
    // Description:
    //------------------------------------------------------------------------------
    class AsaFGMissile : public AsaFGAbstractMissile
    {
        DECLARE_SUBCLASS(AsaFGMissile, AsaFGAbstractMissile)
    public:
        AsaFGMissile();

        asa::models::AsaWeaponData::AsaWeaponType getAsaWeaponType();
        asa::models::AsaWeaponData::AsaWeaponSubtype getAsaWeaponSubtype();
        asa::models::AsaWeaponData::AsaWeaponSensor getAsaWeaponSensor();

        const char *getDescription() const override;
        const char *getNickname() const override;

        double getGrossWeight() const override;
    };
}

#endif /* __asa_AsaFGMissile_H__ */
