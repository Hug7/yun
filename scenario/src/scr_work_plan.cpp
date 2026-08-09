/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_string_utils.h"
#include "c_time_utils.h"
#include "c_constant.h"
#include "standard_csv_reader.h"

/**
 * @brief 解析时间范围并更新日历
 * @param str_time 时间范围字符串 hhmmhhmm
 * @param calendar 日历
 * @param day_ind 日历的日期索引
 */
void parse_time_range_and_update_calendar(const std::string &str_time, Calendar *calendar, const int day_ind)
{
    if (!str_time.empty())
    {
        auto str_time_ranges = StringUtil::split(str_time, Delimiter::CALENDAR_TIME_RANGE);
        auto time_ranges = std::vector<std::pair<int, int>>();
        for (auto &str_time_range : str_time_ranges)
        {
            if (str_time_range.empty())
            {
                continue;
            }
            TimeFromatted::check_hhmmhhmm(str_time_range);
            time_ranges.emplace_back(TimeFromatted::hhmmhhmm_parse_time_range(str_time_range));
        }
        // todo check time ranges overlap
        
        calendar->set_ond_day_time_ranges(day_ind, time_ranges);
    }
}


void StandardCsvReader::loading_work_plan(LocationManager *location_manager, DimensionManager *dimension_manager)
{
    // === loading WorkFixed.csv ===
    spdlog::info("Loading {} ...", WorkFixedSchema::file_name);
    const std::string work_fixed_file_path = this->root_dir + "/" + WorkFixedSchema::file_name;
    FileUtils::file_exists(work_fixed_file_path, WorkFixedSchema::file_name);
    rapidcsv::Document work_fixed_doc(work_fixed_file_path);
    CsvUtils::check_column_exist(work_fixed_doc, WorkFixedSchema::headers, WorkFixedSchema::file_name);
    
    int work_plan_count = work_fixed_doc.GetRowCount();
    for (int u = 0; u < work_plan_count; u++)
    {
        const std::string location_code = work_fixed_doc.GetCell<std::string>(WorkFixedSchema::headers[WorkFixedSchema::LOCATION_CODE], u);
        const int fixed_pick_time = work_fixed_doc.GetCell<int>(WorkFixedSchema::headers[WorkFixedSchema::FIXED_PICK_TIME], u);
        const int fixed_drop_time = work_fixed_doc.GetCell<int>(WorkFixedSchema::headers[WorkFixedSchema::FIXED_DROP_TIME], u);
        Location *location = location_manager->get_location(location_code);
        if (location == nullptr)
        {
            throw std::runtime_error("Location " + location_code + " was not found in " + WorkFixedSchema::file_name);
        }
        location->work_plan->set_fixed_pick_time(fixed_pick_time);
        location->work_plan->set_fixed_drop_time(fixed_drop_time);
    }
    work_fixed_doc.Clear();
    spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.", 
        WorkFixedSchema::file_name, work_plan_count);

    // === loading WorkEffectSchema.csv ===
    spdlog::info("Loading {} ...", WorkEffectSchema::file_name);
    const std::string work_effect_file_path = this->root_dir + "/" + WorkEffectSchema::file_name;
    FileUtils::file_exists(work_effect_file_path, WorkEffectSchema::file_name);
    rapidcsv::Document work_effect_doc(work_effect_file_path);
    CsvUtils::check_column_exist(work_effect_doc, WorkEffectSchema::headers, WorkEffectSchema::file_name);
    
    int work_effect_count = work_effect_doc.GetRowCount();
    for (int u = 0; u < work_effect_count; u++)
    {
        const std::string location_code = work_effect_doc.GetCell<std::string>(WorkEffectSchema::headers[WorkEffectSchema::LOCATION_CODE], u);
        const std::string dimension_code = work_effect_doc.GetCell<std::string>(WorkEffectSchema::headers[WorkEffectSchema::DIMENSION_CODE], u);
        const double per_hour_process_quantity = work_effect_doc.GetCell<double>(WorkEffectSchema::headers[WorkEffectSchema::PER_HOUR_PROCESS_QUANTITY], u);
        Location *location = location_manager->get_location(location_code);
        if (location == nullptr)
        {
            throw std::runtime_error("Location " + location_code + " was not found in " + WorkEffectSchema::file_name);
        }
        Dimension *dimension = dimension_manager->get_dimension(dimension_code);
        if (dimension == nullptr)
        {
            throw std::runtime_error("Dimension " + dimension_code + " was not found in " + WorkEffectSchema::file_name);
        }

        location->work_plan->set_work_effect(new WorkEffect(dimension, per_hour_process_quantity));
    }
    work_effect_doc.Clear();
    spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.", 
        WorkEffectSchema::file_name, work_effect_count);

    // === WorkCalendarDay.csv ===
    spdlog::info("Loading {} ...", WorkCalendarDailySchema::file_name);
    const std::string work_calendar_daily_file_path = this->root_dir + "/" + WorkCalendarDailySchema::file_name;
    FileUtils::file_exists(work_calendar_daily_file_path, WorkCalendarDailySchema::file_name);
    rapidcsv::Document work_calendar_daily_doc(work_calendar_daily_file_path);
    CsvUtils::check_column_exist(work_calendar_daily_doc, WorkCalendarDailySchema::headers, WorkCalendarDailySchema::file_name);

    int work_calendar_day_count = work_calendar_daily_doc.GetRowCount();
    for (int u = 0; u < work_calendar_day_count; u++)
    {
        const std::string location_code = work_calendar_daily_doc.GetCell<std::string>(WorkCalendarDailySchema::headers[WorkCalendarDailySchema::LOCATION_CODE], u);
        const std::string calendar_type = work_calendar_daily_doc.GetCell<std::string>(WorkCalendarDailySchema::headers[WorkCalendarDailySchema::CALENDAR_TYPE], u);
        const std::string daily = work_effect_doc.GetCell<std::string>(WorkCalendarDailySchema::headers[WorkCalendarDailySchema::DAILY], u);
        
        Location *location = location_manager->get_location(location_code);
        if (location == nullptr)
        {
            throw std::runtime_error("Location " + location_code + " was not found in " + WorkCalendarDailySchema::file_name);
        }
        auto calendar_type_it = CalendarTypeMap.find(calendar_type);
        if (calendar_type_it == CalendarTypeMap.end())
        {
            throw std::runtime_error("CalendarType " + calendar_type + " was not support in " + WorkCalendarDailySchema::file_name);
        }
        CalendarType calendar_type_enum = calendar_type_it->second;
        Calendar *calendar = new Calendar(CalendarTimeRangeType::DAILY);

        parse_time_range_and_update_calendar(daily, calendar, 0);

        location->work_plan->set_calendar(calendar, calendar_type_enum);
    }
    work_calendar_daily_doc.Clear();
    spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.", 
        WorkCalendarDailySchema::file_name, work_calendar_day_count);
    
    // === WorkCalendarWeek.csv ===
    const std::string work_calendar_weekly_file_path = this->root_dir + "/" + WorkCalendarWeeklySchema::file_name;
    FileUtils::file_exists(work_calendar_weekly_file_path, WorkCalendarWeeklySchema::file_name);
    rapidcsv::Document work_calendar_weekly_doc(work_calendar_weekly_file_path);
    CsvUtils::check_column_exist(work_calendar_weekly_doc, WorkCalendarWeeklySchema::headers, WorkCalendarWeeklySchema::file_name);
    int work_calendar_week_count = work_calendar_weekly_doc.GetRowCount();
    for (int u = 0; u < work_calendar_week_count; u++)
    {
        const std::string location_code = work_calendar_weekly_doc.GetCell<std::string>(WorkCalendarWeeklySchema::headers[WorkCalendarWeeklySchema::LOCATION_CODE], u);
        const std::string calendar_type = work_calendar_weekly_doc.GetCell<std::string>(WorkCalendarWeeklySchema::headers[WorkCalendarWeeklySchema::CALENDAR_TYPE], u);
        const std::string monday = work_calendar_weekly_doc.GetCell<std::string>(WorkCalendarWeeklySchema::headers[WorkCalendarWeeklySchema::MONDAY], u);
        const std::string tuesday = work_calendar_weekly_doc.GetCell<std::string>(WorkCalendarWeeklySchema::headers[WorkCalendarWeeklySchema::TUESDAY], u);
        const std::string wednesday = work_calendar_weekly_doc.GetCell<std::string>(WorkCalendarWeeklySchema::headers[WorkCalendarWeeklySchema::WEDNESDAY], u);
        const std::string thursday = work_calendar_weekly_doc.GetCell<std::string>(WorkCalendarWeeklySchema::headers[WorkCalendarWeeklySchema::THURSDAY], u);
        const std::string friday = work_calendar_weekly_doc.GetCell<std::string>(WorkCalendarWeeklySchema::headers[WorkCalendarWeeklySchema::FRIDAY], u);
        const std::string saturday = work_calendar_weekly_doc.GetCell<std::string>(WorkCalendarWeeklySchema::headers[WorkCalendarWeeklySchema::SATURDAY], u);
        const std::string sunday = work_calendar_weekly_doc.GetCell<std::string>(WorkCalendarWeeklySchema::headers[WorkCalendarWeeklySchema::SUNDAY], u);
        Location *location = location_manager->get_location(location_code);
        if (location == nullptr)
        {
            throw std::runtime_error("Location " + location_code + " was not found in " + WorkCalendarWeeklySchema::file_name);
        }

        auto calendar_type_it = CalendarTypeMap.find(calendar_type);
        if (calendar_type_it == CalendarTypeMap.end())
        {
            throw std::runtime_error("CalendarType " + calendar_type + " was not support in " + WorkCalendarWeeklySchema::file_name);
        }
        CalendarType calendar_type_enum = calendar_type_it->second;
        Calendar *calendar = new Calendar(CalendarTimeRangeType::WEEKLY);

        parse_time_range_and_update_calendar(monday, calendar, 0);
        parse_time_range_and_update_calendar(tuesday, calendar, 1);
        parse_time_range_and_update_calendar(wednesday, calendar, 2);
        parse_time_range_and_update_calendar(thursday, calendar, 3);
        parse_time_range_and_update_calendar(friday, calendar, 4);
        parse_time_range_and_update_calendar(saturday, calendar, 5);
        parse_time_range_and_update_calendar(sunday, calendar, 6);

        location->work_plan->set_calendar(calendar, calendar_type_enum);
    }
    work_calendar_weekly_doc.Clear();
    spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.", 
        WorkCalendarWeeklySchema::file_name, work_calendar_week_count);
}