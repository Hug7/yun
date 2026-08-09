/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <array>

class ParameterSchema
{
public:
    static constexpr char file_name[] = "Parameter.csv";

    enum Col
    {
        CODE,
        VALUE,
    };

    static constexpr std::array headers{
        "Code",
        "Value",
    };
};

class CargoOrderSchema
{
public:
    static constexpr char file_name[] = "CargoOrder.csv";

    enum Col
    {
        CODE,
        NAME,
        PICK_LOCATION_CODE,
        DROP_LOCATION_CODE,
        EARLIEST_PICK_DATE_TIME,
        LATEST_PICK_DATE_TIME,
        EARLIEST_DROP_DATE_TIME,
        LATEST_DROP_DATE_TIME,
    };

    static constexpr std::array headers{
        "Code",
        "Name",
        "PickLocationCode",
        "DropLocationCode",
        "EarliestPickDateTime",
        "LatestPickDateTime",
        "EarliestDropDateTime",
        "LatestDropDateTime",
    };
};

class CargoSubOrderSchema
{
public:
    static constexpr char file_name[] = "CargoSubOrder.csv";

    enum Col
    {
        CARGO_ORDER_CODE,
        CARGO_SUB_ORDER_CODE,
        CARGO_SUB_ORDER_NAME,
        QUANTITY,
    };

    static constexpr std::array headers{
        "CargoOrderCode",
        "CargoSubOrderCode",
        "CargoSubOrderName",
        "Quantity",
    };
};

class CargoSubOrderDimensionValueSchema
{
public:
    static constexpr char file_name[] = "CargoSubOrderDimensionValue.csv";

    enum Col
    {
        CARGO_ORDER_CODE,
        CARGO_SUB_ORDER_CODE,
        DIMENSION_CODE,
        DIMENSION_VALUE,
    };

    static constexpr std::array headers{
        "CargoOrderCode",
        "CargoSubOrderCode",
        "DimensionCode",
        "DimensionValue",
    };
};

class CargoSubOrderLabelValueSchema
{
public:
    static constexpr char file_name[] = "CargoSubOrderLabelValue.csv";

    enum Col
    {
        CARGO_ORDER_CODE,
        CARGO_SUB_ORDER_CODE,
        LABEL_CODE,
        LABEL_VALUE,
    };

    static constexpr std::array headers{
        "CargoOrderCode",
        "CargoSubOrderCode",
        "LabelCode",
        "LabelValue",
    };
};

class CarrierSchema
{
public:
    static constexpr char file_name[] = "Carrier.csv";

    enum Col
    {
        CODE,
        NAME,
    };

    static constexpr std::array headers{
        "Code",
        "Name",
    };
};

class CarrierLabelValueSchema
{
public:
    static constexpr char file_name[] = "CarrierLabelValue.csv";

    enum Col
    {
        CARRIER_CODE,
        LABEL_CODE,
        LABEL_VALUE,
    };

    static constexpr std::array headers{
        "CarrierCode",
        "LabelCode",
        "LabelValue",
    };
};

class DimensionSchema
{
public:
    static constexpr char file_name[] = "Dimension.csv";

    enum Col
    {
        CODE,
        NAME,
        PRECISION,
    };

    static constexpr std::array headers{
        "Code",
        "Name",
        "Precision",
    };
};

class DistMatrixCodeSchema
{
public:
    static constexpr char file_name[] = "DistMatrixCode.csv";

    enum Col
    {
        CODE,
        NAME,
    };

    static constexpr std::array headers{
        "Code",
        "Name",
    };
};

class DistMatrixSchema
{
public:
    static constexpr char file_name[] = "DistMatrix.csv";

    enum Col
    {
        FROM_LOCATION_CODE,
        TO_LOCATION_CODE,
        DIST_MATRIX_CODE,
        DISTANCE,
        TIME,
    };

    static constexpr std::array headers{
        "FromLocationCode",
        "ToLocationCode",
        "DistMatrixCode",
        "Distance",
        "Time",
    };
};

class LabelSchema
{
public:
    static constexpr char file_name[] = "Label.csv";

    enum Col
    {
        CODE,
        NAME,
    };

    static constexpr std::array headers{
        "Code",
        "Name",
    };
};

class LabelApplySchema
{
public:
    static constexpr char file_name[] = "LabelApply.csv";

    enum Col
    {
        LABEL_CODE,
        APPLY_ITEM,
    };

    static constexpr std::array headers{
        "LabelCode",
        "ApplyItem",
    };
};

class LabelValueSchema
{
public:
    static constexpr char file_name[] = "LabelValue.csv";

    enum Col
    {
        LABEL_CODE,
        LABEL_VALUE,
    };

    static constexpr std::array headers{
        "LabelCode",
        "LabelValue",
    };
};

class LocationSchema
{
public:
    static constexpr char file_name[] = "Location.csv";

    enum Col
    {
        CODE,
        NAME,
        LNG,
        LAT,
    };

    static constexpr std::array headers{
        "Code",
        "Name",
        "Lng",
        "Lat",
    };
};

class LocationAvailableVehicleSchema
{
public:
    static constexpr char file_name[] = "LocationAvailableVehicle.csv";

    enum Col
    {
        LOCATION_CODE,
        CARRIER_CODE,
        VEHICLE_MODEL_CODE,
    };

    static constexpr std::array headers{
        "LocationCode",
        "CarrierCode",
        "VehicleModelCode",
    };
};

class LocationLabelValueSchema
{
public:
    static constexpr char file_name[] = "LocationLabelValue.csv";

    enum Col
    {
        LOCATION_CODE,
        LABEL_CODE,
        LABEL_VALUE,
    };

    static constexpr std::array headers{
        "LocationCode",
        "LabelCode",
        "LabelValue",
    };
};

class VehicleSchema
{
public:
    static constexpr char file_name[] = "Vehicle.csv";

    enum Col
    {
        CARRIER_CODE,
        VEHICLE_MODEL_CODE,
        COUNT,
        ORIGIN_LOCATION_CODE,
        DESTINATION_LOCATION_CODE,
    };

    static constexpr std::array headers{
        "CarrierCode",
        "VehicleModelCode",
        "Count",
        "OriginLocationCode",
        "DestinationLocationCode",
    };
};

class VehicleModelSchema
{
public:
    static constexpr char file_name[] = "VehicleModel.csv";

    enum Col
    {
        CODE,
        NAME,
        DIST_MATRIX_CODE,
    };

    static constexpr std::array headers{
        "Code",
        "Name",
        "DistMatrixCode",
    };
};

class VehicleModelDimensionValueSchema
{
public:
    static constexpr char file_name[] = "VehicleModelDimensionValue.csv";

    enum Col
    {
        VEHICLE_MODEL_CODE,
        DIMENSION_CODE,
        DIMENSION_VALUE,
    };

    static constexpr std::array headers{
        "VehicleModelCode",
        "DimensionCode",
        "DimensionValue",
    };
};

class VehicleModelLabelValueSchema
{
public:
    static constexpr char file_name[] = "VehicleModelLabelValue.csv";

    enum Col
    {
        VEHICLE_MODEL_CODE,
        LABEL_CODE,
        LABEL_VALUE,
    };

    static constexpr std::array headers{
        "VehicleModelCode",
        "LabelCode",
        "LabelValue",
    };
};

class WorkCalendarDailySchema
{
public:
    static constexpr char file_name[] = "WorkCalendarDaily.csv";

    enum Col
    {
        LOCATION_CODE,
        CALENDAR_TYPE,
        DAILY,
    };

    static constexpr std::array headers{
        "LocationCode",
        "CalendarType",
        "Daily",
    };
};

class WorkCalendarWeeklySchema
{
public:
    static constexpr char file_name[] = "WorkCalendarWeekly.csv";

    enum Col
    {
        LOCATION_CODE,
        CALENDAR_TYPE,
        MONDAY,
        TUESDAY,
        WEDNESDAY,
        THURSDAY,
        FRIDAY,
        SATURDAY,
        SUNDAY,
    };

    static constexpr std::array headers{
        "LocationCode",
        "CalendarType",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
        "Sunday",
    };
};

class WorkEffectSchema
{
public:
    static constexpr char file_name[] = "WorkEffect.csv";

    enum Col
    {
        LOCATION_CODE,
        DIMENSION_CODE,
        PER_HOUR_PROCESS_QUANTITY,
    };

    static constexpr std::array headers{
        "LocationCode",
        "DimensionCode",
        "PerHourProcessQuantity",
    };
};

class WorkFixedSchema
{
public:
    static constexpr char file_name[] = "WorkFixed.csv";

    enum Col
    {
        LOCATION_CODE,
        FIXED_PICK_TIME,
        FIXED_DROP_TIME,
    };

    static constexpr std::array headers{
        "LocationCode",
        "FixedPickTime",
        "FixedDropTime",
    };
};